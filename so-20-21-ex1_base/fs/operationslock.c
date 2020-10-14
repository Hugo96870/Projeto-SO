#ifndef FS_H
#define FS_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

extern pthread_mutex_t lockm;
extern pthread_rwlock_t lockrw;
extern char p[6];

void close(char *aux){
	if(strcmp(p,"mutex")==0){
		pthread_mutex_lock(&lockm);
	}
	else if(strcmp(p,"rwlock")== 0 && strcmp(aux,"wr")){
		pthread_rwlock_wrlock(&lockrw);
	}
	else if(strcmp(p,"rwlock")== 0 && strcmp(aux,"rd")){
		pthread_rwlock_rdlock(&lockrw);
	}
	else{
	}
}

void open(){
	if(strcmp(p,"mutex")==0){
		pthread_mutex_unlock(&lockm);
	}
	else if(strcmp(p,"rwlock")== 0){
		pthread_rwlock_unlock(&lockrw);
	}
	else{
	}
}