#ifndef FS_H
#define FS_H
#include "state.h"
#include <pthread.h>

extern pthread_mutex_t lockm;
extern pthread_rwlock_t lockrw;
extern char p[6];

void closelocks(char *aux);
void openlocks();


void init_fs();
void destroy_fs();
int is_dir_empty(DirEntry *dirEntries);
int create(char *name, type nodeType);
int delete(char *name);
int lookup(char *name);
void print_tecnicofs_tree(FILE *fp);

#endif /* FS_H */
