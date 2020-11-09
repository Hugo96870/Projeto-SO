#ifndef FS_H
#define FS_H
#include "state.h"
#include <pthread.h> 

void init_fs();
void destroy_fs();
int is_dir_empty(DirEntry *dirEntries);
int create(char *name, type nodeType, int vetorlocks[], int counter);
int delete(char *name, int vetorlocks[], int counter);
int lookup(char *name, int nr, int vetorlocks[], int counter);
void print_tecnicofs_tree(FILE *fp);

#endif /* FS_H */
