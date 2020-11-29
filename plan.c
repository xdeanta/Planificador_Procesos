#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <signal.h>

#include "list.h"

// Xavier De Anta CI:23634649
// Johan Velasquez CI:21759251

typedef struct _PCB{
	int id;
	int priority;
	int resources[4];
	int status;
	int time_cpu;
	int time_arrival;
	int q_time;
	int exec_status;
} PCB;

list* PL; //Lista inicial de procesos no iniciados
list* PTR; //cola de procesos de tiempo real
list* PUS; //cola de procesos de usuario no listos
list* PS1; //cola de procesos de usuario de prioridad 1
list* PS2; //cola de procesos de usuario de prioridad 2
list* PS3; //cola de procesos de usuario de prioridad 3

int exec_time=1; //Contador de tiempo

int max_time=20; //Temporizador de tiempo de ejecucion
int max_resourse[] = {2, 1, 1, 2}; //Cantidad maxima de recursos
PCB* exec_process; //Proceso en ejecucion

void create_process(char* source); //Funcion que crea la lista inicial de procesos
void read_file(int arg_c, char* fname); //Funcion que lee el archivo y extrae los datos

void dispatch(); //Funcion que despacha los procesos listos a sus respectivas colas
void dispatch_user(); //Funcion que despacha procesos a sus respectivas colas de prioridad
void execute(); //Ejecuta la simulacion del despacho del proceso al procesador
void create_child(PCB*); //funcion que hace fork() y exec() del proceso seleccionado para ejecutar
PCB* select_process(); //funcion que selecciona el proceso para ejecutar

int check_resources(PCB* a); //funcion que revisa si el proceso seleccionado tenga los recursos necesarios para su ejecucion
void assign_resources(PCB* a); //Asigna los recursos al proceso seleccionado
void deassign_resources(PCB* a); //Devuelve los recursos al sistema despues de que el proceso termino

void handler_signal(); //El manejador de senales del planificador
void handler_check_process_time(); //funcion que llama el manejador de senales para el cambio de proceso del procesador
void sort_PUS(); //funcion para ordenar la cola de procesos de usuario por prioridad
void delete_process(PCB* process); //borra el proceso despues de que este termine su ejecucion

//void print_resources(); //Funcion de prueba no relevante

void handler_signal(int signo){
	signal(SIGUSR1, handler_signal); //Se reinstala el manejador de senales si el SO reinicia el manejador por defecto despues de recibir una senal
	//printf("llamada a dispatch()\n");
	printf("exec_time: %d\n", exec_time); //impresion del contador de tiempo
	//exec_time++;
	//printf("exec_time: %d\n", exec_time);
	dispatch(); //Despues de ejecutar un (1) segundo, despacho mas procesos en el tiempo que correspondan entrar
	handler_check_process_time();
	exec_time++;
	//printf("exec_time: %d\n", exec_time);
	//exec_time++;
	//dispatch();
	//exec_time++;
}

void handler_check_process_time(){
	PCB* process;
	process=exec_process;
	exec_process->time_cpu--;
	exec_process->q_time--;
	max_time--;
	/*
	  Si el proceso que esta en ejecucion sobrepasa los 20 segundos,
	  el planificador mata el proceso
	*/
	if(max_time == 0 && exec_process->time_cpu > 0){
		printf("Proceso %d terminado por el planificador\n", process->id);
		kill(process->id, SIGINT);
		/*
		  Aqui se busca la cola a la cual pertenece el proceso
		  para borrarlo del mismo
		*/
		if(list_size(PTR) > 0 && process == list_at(PTR,1)){
			list_pop_front(PTR);
		}
		if(list_size(PS1) > 0 && process == list_at(PS1,1)){
			list_pop_front(PS1);
		}
		if(list_size(PS2) > 0 && process == list_at(PS2,1)){
			list_pop_front(PS2);
		}
		if(list_size(PS3) > 0 && process == list_at(PS3,1)){
			list_pop_front(PS3);
		}
		free(process);
	}
	if(list_size(PS1) > 0 && process==list_at(PS1,1)){
		//Caso si el proceso en ejecucion es un proceso de usuario de prioridad 1
		if(list_size(PTR) > 0){
			/*
			  Si hay un proceso de tiempo real en la cola esperando,
			  el proceso se suspende, guardando su estado, incluyendo
			  su quantum de tiempo restante
			 */
			if(process->time_cpu > 0){
				printf("Proceso %d detenido por proceso de mayor prioridad\n",process->id);
				kill(process->id, SIGSTOP);
				printf("tiempo restante:%d \n",process->time_cpu);
			}
		}else{
			if(process->q_time == 0){
				/* 
				  Si no hay nadie esperando en la cola de tiempo real
				  y el quantum expiro, suspendo el proceso,
				  degrado su prioridad y la muevo a su cola de su respectiva
				  prioridad
				*/
				printf("Proceso %d detenido por quantum\n", process->id);
				kill(process->id, SIGSTOP);
				process->priority++;
				process->q_time=2;
				list_pop_front(PS1);
				list_push_back(PS2,(void*)process);
				printf("tiempo restante:%d \n",process->time_cpu);
			}
		}
	}
	if(list_size(PS2) > 0 && process==list_at(PS2,1)){
		//Caso si el proceso en ejecucion es un proceso de usuario de prioridad 2
		if(list_size(PTR) > 0 || list_size(PS1) > 0){
			if(process->time_cpu > 0){
				printf("Proceso %d detenido por proceso de mayor prioridad\n",process->id);
				kill(process->id, SIGSTOP);
				printf("tiempo restante:%d \n",process->time_cpu);
			}
		}else{
			if(process->q_time == 0){
				printf("Proceso %d detenido por quantum\n", process->id);
				kill(process->id, SIGSTOP);
				process->priority++;
				process->q_time=1;
				list_pop_front(PS2);
				list_push_back(PS3,(void*)process);
				printf("tiempo restante:%d \n",process->time_cpu);
			}
		}
	}
	if(list_size(PS3) > 0 && process==list_at(PS3,1)){
		if((list_size(PTR) > 0) || (list_size(PS1) > 0) || (list_size(PS2) > 0)){
			printf("Proceso %d detenido por proceso de mayor prioridad\n",process->id);
			kill(process->id, SIGSTOP);
			printf("tiempo restante:%d \n",process->time_cpu);
		}else{
			if(process->q_time==0){
				printf("Proceso %d detenido por quantum\n", process->id);
				kill(process->id, SIGSTOP);
				process->q_time=1;
				list_pop_front(PS3);
				list_push_back(PS3,(void*)process);
				printf("tiempo restante:%d \n",process->time_cpu);
			}
		}
	}
	if(max_time==0){ //Si el temporizador expiro, lo reseteo
		max_time=20;
	} //Funcion que llama el manejador para el control y planificacion de los procesos //Funcion que llama el manejador de senales
}

int main(int argc, char** argv){
	int i;
	PCB* process;
	PL=alloc_list();
	PTR=alloc_list();
	PUS=alloc_list();
	PS1=alloc_list();
	PS2=alloc_list();
	PS3=alloc_list();
	read_file(argc, argv[1]);
	signal(SIGUSR1, handler_signal);
	dispatch();
	//exec_time++;
	//dispatch();
	//exec_time=exec_time;
	//dispatch();
	execute();
	//dispatch_user();
	//printf("tamano lista: %d\n",list_size(PL));
	/*for(i = 1;i <= list_size(PL); i++){
		process=(PCB*)list_at(PL,i);
		printf("Prioridad de Proceso %d: %d\n", i, process->priority);
	}*/
	return 0;
}

void create_process(char* source){ //Esta funcion parsea el string y crea un string nuevo para generar un proceso nuevo para despues encolarlo
	PCB* new_process;
	char cpystr[22];
	char* pchar;
	char f_buffer[22];
	int i;
	int ibuffer[7];
	new_process=(PCB*)malloc(sizeof(PCB)); //creo un registro donde se va a guardar la informacion del proceso
	strcpy(cpystr,source); //source el es buffer que se lee del archivo
	pchar=strtok(cpystr, ","); //Se separa el string por ","
	while(pchar != NULL){ //Parseo el string
		strcat(f_buffer, pchar); //Concateno solo los numeros
		pchar=strtok(NULL, ",");
	}
	/*
	  Del string formado del ciclo anterior,
	  transformo los numeros de characteres
	  a numeros enteros con sscanf
	*/
	sscanf(f_buffer, "%d %d %d %d %d %d %d", &ibuffer[0], &ibuffer[1], &ibuffer[2], &ibuffer[3], &ibuffer[4], &ibuffer[5], &ibuffer[6]);
	/*
	  Se guarda la informacion con su respectivo formato de entrada
	*/
	new_process->id=0;
	new_process->status=-1;
	new_process->exec_status=0;
	new_process->time_arrival=ibuffer[0];
	new_process->priority=ibuffer[1];
	new_process->time_cpu=ibuffer[2];
	for (i = 0; i < 4; ++i){
		//printf("new_process->resources[%d]: %d\n", i, ibuffer[i+3]);
		new_process->resources[i]=ibuffer[i+3];
	}
	list_push_back(PL, (void*)new_process);
}

void read_file(int arg_c, char* fname){
	FILE *fp;
	char buffer[22];
	if(arg_c == 1){
		exit(1);
	}else{
		fp=fopen(fname, "r");
		if(fp == NULL){
			printf("Error al abrir el archivo\n");
			exit(1);
		}else{
			while(!feof(fp)){ //Leo el archivo completo con este ciclo
				fgets(buffer, 22, fp); //Obtengo la primera linea;
				if(strcmp(buffer, "\n") == 0){ //comparo si el string solo tenga el caracter \n
					continue; //si solo tiene a \n, salto de ciclo
				}
				//printf("buffer: %s\n", buffer);
				create_process(buffer); //Paso la linea leida a esta funcion
			}
			fclose(fp);
		}
	}
}

void dispatch(){ //Despacha y distribuye los procesos de la lista inicial
	PCB* process;
	//printf("soy dispatch\n");
	//printf("exec_time:%d\n", exec_time);
	while (list_size(PL) > 0){ //Mientras que la lista no este vacia
		process=(PCB*)list_pop_front(PL); //Saco un proceso de la lista que llegen en el tiempo actual de la simulacion
		if(process->time_arrival == exec_time){ //Si el tiempo en el que llega es igual al tiempo actual
			//printf("exec_time: %d\n", exec_time);
			if(process->priority == 0){ //Si es un proceso de tiempo real lo encolo en esa cola
				//printf("push PTR\n");
				list_push_back(PTR,(void*)process);
			}else{ //Sino es un proceso de usuario y se encola en la respectiva cola
				//printf("priority!=0\n");
				list_push_back(PUS, (void*)process);
				//dispatch_user();
			}
		}else{ //Si el tiempo de llegada no es el tiempo actual lo devuelvo a la lista y rompo el ciclo
			list_push_front(PL, (void*)process);
			break;
		}
	}
	dispatch_user();
}

void dispatch_user(){ //Distribuye los procesos de usuario a sus respectivas colas de prioridad
	PCB* process;
	sort_PUS();
	while(list_size(PUS) > 0){
		process=(PCB*)list_pop_front(PUS);
		if(check_resources(process)){ //Si los recursos que solicita el proceso estan disponibles, muevo el proceso a su respectiva cola por su prioridad
			switch(process->priority){
				case 1: //Prioridad 1
					process->q_time=3;
					assign_resources(process); //Le asigno los recursos al proceso
					list_push_back(PS1,(void*) process); //Se encola el proceso
				break;
				case 2: //Prioridad 2
					process->q_time=2;
					assign_resources(process);
					list_push_back(PS2,(void*) process);
				break;
				case 3: //Prioridad 3
					process->q_time=1;
					assign_resources(process);
					list_push_back(PS3,(void*) process);
				break;
			}
		}else{ //Si algun recurso no esta disponible de devuelve a la cola de usuarios y rompo el ciclo
			process->time_arrival++;
			list_push_front(PUS, (void*)process);
			break;
		}
	}
}

int check_resources(PCB* a){ //Funcion que chequea si estan todos los recursos disponibles para el proceso, devuelve 1 si los tienes, 0 en caso contrario
	int i, ret=1;
	for(i = 0; i<4 ;i++){
		if(a->resources[i] > max_resourse[i]){
			ret=0;
			break;
		}
	return ret;
	}
}

void assign_resources(PCB* a){ //Funcion que asigna los recursos a los procesos
	int i;
	for(i = 0; i < 4; i++){
		max_resourse[i]=max_resourse[i] - a->resources[i];
	}
}

void deassign_resources(PCB* a){
	int i;
	for(i = 0; i<4;i++){
		max_resourse[i]=max_resourse[i]+a->resources[i];
	}
}

void execute(){
	PCB* actual_process;
	PCB* next_process;
	char arg_number[2];
	actual_process=NULL;
	next_process=NULL;
	while((list_size(PTR) + list_size(PS1) + list_size(PS2) + list_size(PS3)) > 0){
		/*
		  Mientras que todas las cola esten vacias,
		  Selecciono un proceso respetando su prioridad
		  y las ejecuto hasta que terminen o se bloqueen
		*/

		if(exec_process == NULL){
			/*
			  Si no hay procesos en ejecucion,
			  selecciono un proceso que este esperando
			  en las colas de prioridades
			*/
			actual_process=select_process();
			exec_process=actual_process;
			/*
			  Si es un proceso que nunca se ha ejecutado,
			  lo creo con create_child
			*/
			if(exec_process->id == 0){
				//print_resources();
				create_child(exec_process);
			}
			/*
			  Aqui espero hasta que el proceso que se
			  termine de ejecutar o se bloquee
			*/
			waitpid(exec_process->id,&exec_process->status, WUNTRACED);
			if(WIFEXITED(exec_process->status)){
				/*
				  Si ya termino, notifico que ese proceso
				  termino. Si es un proceso de usuario,
				  devuelvo los recursos al planificador
				  antes de borrarlo de la cola
				*/
				printf("Proceso %d finalizo\n", exec_process->id);
				if(exec_process->priority > 0){
					deassign_resources(exec_process);
				}
				//print_resources();
				delete_process(exec_process);
				//exec_process==NULL;
			}
			//printf("exec_process = null, exec_time:%d\n", exec_time);
		}else{
			/*
			  Caso del proceso que ya se ejecuto alguna vez.
			  exec_process por alguna estrana razon,
			  no es NULL y tuve que validar ese caso
			  con la creacion del proceso;
			*/
			next_process=select_process();
			exec_process=next_process;
			if(exec_process->id == 0){
				create_child(exec_process);
			}else{
				/* 
				  Si el proceso se ejecuto alguna vez,
				  Simplemente reanudo el proceso que se bloqueo
				*/
				kill(exec_process->id, SIGCONT);
				printf("Proceso %d reanudado\n", exec_process->id);
			}
			waitpid(exec_process->id,&exec_process->status, WUNTRACED);
			if(WIFEXITED(exec_process->status)){
				printf("Proceso %d finalizo\n", exec_process->id);
				if(exec_process->priority > 0){
					deassign_resources(exec_process);
				}
				//print_resources();
				delete_process(exec_process);
			}
			//printf("exec_process != null, exec_time:%d\n", exec_time);
		}
	}
}

PCB* select_process(){
	PCB* a;
	a=NULL;
	/*
	  Selecciono el proceso con mayor prioridad
	  de la cabeza de alguna de las colas
	*/
	if(list_size(PTR) > 0){
		a=(PCB*)list_at(PTR,1);
	}else{
		if(list_size(PS1) > 0){
			a=(PCB*)list_at(PS1, 1);
		}else{
			if(list_size(PS2) > 0){
				a=(PCB*)list_at(PS2,1);
			}else{
				if(list_size(PS3) > 0){
					a=(PCB*)list_at(PS3,1);
				}
			}
		}
	}
	return a;
}

void create_child(PCB* process){
	char arg_number[2]; //el argumento de va al proceso child para llamar a exec()
	if((process->id = fork()) == 0){
		printf("Proceso ID: %d\n", getpid());
		printf("Prioridad: %d\n", process->priority);
		printf("Tiempo restante: %d\n", process->time_cpu);
		printf("Impresoras: %d, Scanner: %d, Modem: %d, DVDs: %d\n", process->resources[0], process->resources[1], process->resources[2], process->resources[3]);
		sprintf(arg_number,"%d", process->time_cpu);
		if(execlp("./child", "child", arg_number, NULL) < 0){
			printf("Error execlp()\n");
		}
	}
}

void delete_process(PCB* process){
	/*
	  Busco la cola a la cual pertenece el proceso especificado
	  en el parametro para borrarlo de la misma
	*/
	if(list_size(PTR) > 0 && process == list_at(PTR,1)){
		list_pop_front(PTR);
		free(process);
	}else{
		if(list_size(PS1) > 0 && process == list_at(PS1,1)){
			list_pop_front(PS1);
			free(process);
		}else{
			if(list_size(PS2) > 0 && process == list_at(PS2,1)){
				list_pop_front(PS2);
				free(process);
			}else{
				if(list_size(PS3) > 0 && process == list_at(PS3,1)){
					list_pop_front(PS3);
					free(process);
				}
			}
		}
	}
	process=NULL;
}

void sort_PUS(){ //Funcion que ordena la cola de usuarios
	list* p1;
	list* p2;
	list* p3;
	//colas auxiliares
	PCB* process;
	p1=alloc_list();
	p2=alloc_list();
	p3=alloc_list();
	while(list_size(PUS) > 0){
		/*
		  Distribuyo el contenido de la cola de usuarios
		  a las colas auxiliares por prioridad
		*/
		process=list_pop_front(PUS);
		if(process->priority == 1){
			list_push_back(p1,(void*)process);
		}
		if(process->priority == 2){
			list_push_back(p2,(void*)process);
		}
		if(process->priority == 3){
			list_push_back(p3,(void*)process);
		}
	}
	/*
	  Esos ciclos insertan los elementos otra vez
	  a la cola de usuarios original, colocando
	  adelante los procesos con mayor prioridad
	*/
	while(list_size(p1) > 0){
		process=list_pop_front(p1);
		list_push_back(PUS, (void*)process);
	}
	while(list_size(p2) > 0){
		process=list_pop_front(p2);
		list_push_back(PUS,(void*)process);
	}
	while(list_size(p3) > 0){
		process=list_pop_front(p3);
		list_push_back(PUS,(void*)process);
	}
	free(p1);
	free(p2);
	free(p3);
}

/*void print_resources(){
	int i;
	for(i=0;i<4;i++){
		printf("max_resourse[%d]: %d ",i,max_resourse[i]);
	}
	printf("\n");
}*/
