#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
int strtoint(char* num){
	int  i, len;
	int result=0;
	len = strlen(num);
	for(i=0; i<len; i++){
		result = result * 10 + ( num[i] - '0' );
	}
	return result;
}

void work( char *num ) {
	int i,N = strtoint(num);
	for (i = 0; i < N; i++ ){
		printf("Ejecucion de %d\n", getpid());
		kill(getppid(), SIGUSR1); //senal que le envia al padre que va a dormir el proceso
		sleep(1);
	}
	return;
}

int main(int argv, char **argc ) {
	if ( argv != 2 ) return -1;
	else	work(argc[1]);
	return 0;
}
