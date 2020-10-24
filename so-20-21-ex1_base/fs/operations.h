#ifndef FS_H
#define FS_H
#include "state.h"
#include <pthread.h> 

void closelocks(char *aux,pthread_rwlock_t lock, pthread_rwlock_t vetorlocks[], int counter);
void openlocks();
void init_fs();
void destroy_fs();
int is_dir_empty(DirEntry *dirEntries);
int create(char *name, type nodeType, pthread_rwlock_t vetorlocks[], int counter);
int delete(char *name, pthread_rwlock_t vetorlocks[], int counter);
int lookup(char *name, int nr, pthread_rwlock_t vetorlocks[], int counter);
void print_tecnicofs_tree(FILE *fp);

#endif /* FS_H */
