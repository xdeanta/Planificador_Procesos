#ifndef __LIST_H
#define __LIST_H

/*
  Lista general de apuntadores a void*
  Implementada por Xavier De Anta. CI: 23634649 y Johan Velasquez CI:21759251
*/

typedef struct _node{
	void* item;
	struct _node* next;
	struct _node* prev;
} node;

typedef struct _list{
	node* first;
	node* last;
	unsigned int size;
} list;

list* alloc_list(void){
	list* l;
	l=(list*)malloc(sizeof(list));
	l->first=NULL;
	l->last=NULL;
	l->size=0;
}

int list_size(list* l){
	return l->size;
}

void list_insert_at(list* l, int pos, void* data){
	node* new_node;
	new_node=(node*)malloc(sizeof(node));
	//new_node->item=malloc(sizeof(void*));
	new_node->item=data;
	//printf("new node data:%d\n", *(int*)new_node->item);
	//printf("l->first:%d\n", *(int*)l->first->item);
	new_node->next=NULL;
	new_node->prev=NULL;
	//printf("pos: %d\n", pos);	
	if(list_size(l) == 0){
		//printf("caso vacio\n");
		l->first=new_node;
		l->last=new_node;
	}else{
		if(pos == 0){
			//printf("primer caso\n");
			new_node->prev=l->first;
			l->first->next=new_node;
			l->first=new_node;
			//l->last=new_node;
		}else{
			if(pos > 1 && pos < list_size(l)){
				node* aux1;
				node* aux2;
				int i;
				aux1=l->first;
				for(i = 0; i < pos ;i++){
					aux1=aux1->prev;
				}
					aux2=aux1;
					aux2=aux2->next;
					new_node->next=aux2;
					new_node->prev=aux1;
					aux1->next=new_node;
					aux2->prev=new_node;
			}else{
				if(pos == list_size(l)){
					//printf("ultimo caso\n");
					new_node->next=l->last;
					l->last->prev=new_node;
					l->last=new_node;
				}
			}
		}
	}
	//l->last=new_node;
	l->size++;
	//printf("l->first:%d\n", *(int*)l->first->item);
}


void* list_at(list* l, int pos){
	int i;
	node* aux;
	//aux=l->first;
	//printf("list_at Ciclo\n");
	if(pos >= 1 && pos <= list_size(l)){
		if(pos == 1){
			aux=l->first;
		}else{
			if(pos == list_size(l)){
				aux=l->last;
			}else{
				aux=l->first;
				for(i = 1; i < pos; i++){
				//printf("aux->item:%d\n", *(int*)aux->item);
				//printf("ciclo aqui\n");
					aux=aux->prev;
				}
			}
		}
	}else{
		printf("Error: Numero fuera de rango\n");
		exit(1);
	}

	/*for(i = 0; i < pos; i++){
		//printf("aux->item:%d\n", *(int*)aux->item);
		//printf("ciclo aqui\n");
		aux=aux->prev;
	}*/
	//printf("aux->item:%d\n", *(int*)aux->item);
	return aux->item;
}

void* list_pop_at(list* l, int pos){
	void* item;
	node* ret;
	if(list_size(l) > 0){
		if(pos >= 1 && pos <= list_size(l)){
			if(pos == 1){
				//printf("pop front item: %d\n", *(int*)l->first->item);
				ret=l->first;
				item=ret->item;
				l->first=l->first->prev;
			}else{
				if(pos > 1 && pos < list_size(l)){
					int i;
					node* aux;
					node* aux2;
					ret=l->first;
					for(i = 1; i < pos ; i++){
						ret=ret->prev;
					}
					aux=ret;
					aux2=ret;
					aux=aux->prev;
					aux2=aux2->next;
					aux->next=aux2;
					aux2->prev=aux;
					item=ret->item;
				}else{
					if(pos == list_size(l)){
						ret=l->last;
						item=ret->item;
						l->last=l->last->next;
					}
				}
			}
			l->size--;
			free(ret);
			if(list_size(l)	== 0){
				l->first=NULL;
				l->last=NULL;
			}
		}
	}else{
		return NULL;
	}
	return item;
}

void free_list(list* l){
	while(list_size(l) > 0){
		list_pop_at(l,1);
	}
	free(l);
}

void* list_pop_front(list* l){
	return list_pop_at(l,1);
}

void* list_pop_back(list* l){
	return list_pop_at(l,list_size(l));
}

void list_push_front(list* l, void* data){
	list_insert_at(l,0,data);
}

void list_push_back(list* l, void* data){
	list_insert_at(l,list_size(l),data);
}

void* list_head(list* l){
	return list_at(l,1);
}

void* list_tail(list* l){
	return list_at(l,list_size(l));
}

#endif
