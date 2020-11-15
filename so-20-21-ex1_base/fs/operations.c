#include "operations.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#define MOVE 3
#define WRITE 1
#define READ 0
#define LER 1
#define ERROR -1
#define CREATE 2

extern inode_t inode_table[INODE_TABLE_SIZE];

/* Closes locks according the sync strategy 
 * Input: 
 *  - aux: describes the critical section as a read or write critical section
 * 	MUDAERRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRR
 */

void closelocks(int state, pthread_rwlock_t *lock, int vetorlocks[], int inumber, int *counter){
	if(state == 1){
		int err = pthread_rwlock_wrlock(lock);
		/*printf("vou fechar escrever%d\n",inumber);*/
		if(err != 0){
			printf("Error: Failed to close lock. %d \n", err);
			exit(EXIT_FAILURE);
		}
	}
	else if(state == 0){
		int err = pthread_rwlock_rdlock(lock);
		/*printf("vou fechar ler%d\n",inumber);*/
		if(err != 0){
			printf("Error: Failed to close lock. %d \n", err);
			exit(EXIT_FAILURE);
		}
	}
	vetorlocks[*counter] = inumber;
	*counter += 1;
}

/* Open locks 
 * Input: 
 *  - aux: describes the critical section as a read or write critical section
 * MUDARRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRR
 */

void openlocks(int vetorlocks[], int *counter){
	int j = 0;
	while(j < *counter){
		if(pthread_rwlock_unlock(&inode_table[vetorlocks[j]].lock) != 0){
			printf("Error: Could not open lock\n");
			exit(EXIT_FAILURE);
		}
		j += 1;
	}
}
int isInVector(int inumber, int locksVector[], int *counter){
	int i;
	for (i = 0; i<*counter; i++){
		if (inumber == locksVector[i])
			return 1;
	}
	return 0;
}

/* Given a path, fills pointers with strings for the parent path and child
 * file name
 * Input:
 *  - path: the path to split. ATENTION: the function may alter this parameter
 *  - parent: reference to a char*, to store parent path
 *  - child: reference to a char*, to store child file name
 */
void split_parent_child_from_path(char * path, char ** parent, char ** child) {

	int n_slashes = 0, last_slash_location = 0;
	int len = strlen(path);

	// deal with trailing slash ( a/x vs a/x/ )
	if (path[len-1] == '/') {
		path[len-1] = '\0';
	}

	for (int i=0; i < len; ++i) {
		if (path[i] == '/' && path[i+1] != '\0') {
			last_slash_location = i;
			n_slashes++;
		}
	}

	if (n_slashes == 0) { // root directory
		*parent = "";
		*child = path;
		return;
	}

	path[last_slash_location] = '\0';
	*parent = path;
	*child = path + last_slash_location + 1;
}


/*
 * Initializes tecnicofs and creates root node.
 */
void init_fs() {
	inode_table_init();

	/* create root inode */
	int root = inode_create(T_DIRECTORY);
	
	if (root != FS_ROOT) {
		printf("failed to create node for tecnicofs root\n");
		exit(EXIT_FAILURE);
	}
}


/*
 * Destroy tecnicofs and inode table.
 */
void destroy_fs() {
	inode_table_destroy();
}


/*
 * Checks if content of directory is not empty.
 * Input:
 *  - entries: entries of directory
 * Returns: SUCCESS or FAIL
 */

int is_dir_empty(DirEntry *dirEntries) {
	if (dirEntries == NULL) {
		return FAIL;
	}
	for (int i = 0; i < MAX_DIR_ENTRIES; i++) {
		if (dirEntries[i].inumber != FREE_INODE) {
			return FAIL;
		}
	}
	return SUCCESS;
}

/*
 * Looks for node in directory entry from name.
 * Input:
 *  - name: path of node
 *  - entries: entries of directory
 * Returns:
 *  - inumber: found node's inumber
 *  - FAIL: if not found
 */

int lookup_sub_node(char *name, DirEntry *entries) {
	if (entries == NULL) {
		return FAIL;
	}

	for (int i = 0; i < MAX_DIR_ENTRIES; i++) {
        if (entries[i].inumber != FREE_INODE && strcmp(entries[i].name, name) == 0) {
            return entries[i].inumber;
        }
    }
	return FAIL;
}

/*
 * Creates a new node given a path.
 * Input:
 *  - name: path of node
 *  - nodeType: type of node
 * Returns: SUCCESS or FAIL
 */
int create(char *name, type nodeType){

	int *vetorlocks = malloc(sizeof(int) * 50);
    int *counter = malloc(sizeof(int));
	*counter = 0;
	int parent_inumber, child_inumber;
	char *parent_name, *child_name, name_copy[MAX_FILE_NAME];

	/* use for copy */
	type pType;
	union Data pdata;

	strcpy(name_copy, name);
	split_parent_child_from_path(name_copy, &parent_name, &child_name);

	parent_inumber = lookup(parent_name, 0, vetorlocks, counter);
	
	if (parent_inumber == FAIL) {
		printf("failed to create %s, invalid parent dir %s\n",
		        name, parent_name);
		openlocks(vetorlocks, counter);
		return FAIL;
	}

	inode_get(parent_inumber, &pType, &pdata);


	if(pType != T_DIRECTORY) {
		printf("failed to create %s, parent %s is not a dir\n",
		        name, parent_name);
		openlocks(vetorlocks, counter);
		return FAIL;
	}

	if (lookup_sub_node(child_name, pdata.dirEntries) != FAIL) {
		printf("failed to create %s, already exists in dir %s\n",
		       child_name, parent_name);
		openlocks(vetorlocks, counter);
		return FAIL;
	}
	/* create node and add entry to folder that contains new node */
	child_inumber = inode_create(nodeType);

	if (child_inumber == FAIL) {
		printf("failed to create %s in  %s, couldn't allocate inode\n",
		        child_name, parent_name);
		openlocks(vetorlocks, counter);
		return FAIL;
	}

	if (dir_add_entry(parent_inumber, child_inumber, child_name) == FAIL) {
		printf("could not add entry %s in dir %s\n",
		       child_name, parent_name);
		openlocks(vetorlocks, counter);
		return FAIL;
	}
	openlocks(vetorlocks, counter);
	return SUCCESS;
}


/*
 * Deletes a node given a path.
 * Input:
 *  - name: path of node
 * Returns: SUCCESS or FAIL
 */
int delete(char *name){
	
	int *vetorlocks = malloc(sizeof(int) * 50);
    int *counter = malloc(sizeof(int));
	*counter = 0;
	int parent_inumber, child_inumber;
	char *parent_name, *child_name, name_copy[MAX_FILE_NAME];

	/* use for copy */
	type pType, cType;
	union Data pdata, cdata;

	strcpy(name_copy, name);
	split_parent_child_from_path(name_copy, &parent_name, &child_name);
	parent_inumber = lookup(parent_name, 2, vetorlocks, counter);

	if (parent_inumber == FAIL) {
		printf("failed to delete %s, invalid parent dir %s\n",
		        child_name, parent_name);
		openlocks(vetorlocks, counter);
		return FAIL;
	}

	inode_get(parent_inumber, &pType, &pdata);

	if(pType != T_DIRECTORY) {
		printf("failed to delete %s, parent %s is not a dir\n",
		        child_name, parent_name);
		openlocks(vetorlocks, counter);
		return FAIL;
	}


	child_inumber = lookup_sub_node(child_name, pdata.dirEntries);

	if (child_inumber == FAIL) {
		printf("could not delete %s, does not exist in dir %s\n",
		       name, parent_name);
		openlocks(vetorlocks, counter);
		return FAIL;
	}

	closelocks(WRITE, &inode_table[child_inumber].lock, vetorlocks, child_inumber, counter);

	inode_get(child_inumber, &cType, &cdata);

	if (cType == T_DIRECTORY && is_dir_empty(cdata.dirEntries) == FAIL) {
		printf("could not delete %s: is a directory and not empty\n",
		       name);
		openlocks(vetorlocks, counter);
		return FAIL;
	}

	/* remove entry from folder that contained deleted node */
	if (dir_reset_entry(parent_inumber, child_inumber) == FAIL) {
		printf("failed to delete %s from dir %s\n",
		       child_name, parent_name);
		openlocks(vetorlocks, counter);
		return FAIL;
	}

	if (inode_delete(child_inumber) == FAIL) {
		printf("could not delete inode number %d from dir %s\n",
		       child_inumber, parent_name);
		openlocks(vetorlocks, counter);
		return FAIL;
	}
	openlocks(vetorlocks, counter);
	
	return SUCCESS;
}

int move(char *name1, char *name2){

    int *vetorlocks = malloc(sizeof(int) * 50);
    int *counter = malloc(sizeof(int));
    *counter = 0;
    int parent_inumber1, parent_inumber2, child_inumber1;
    char *parent_name1, *parent_name2, *child_name1, *child_name2, name_copy1[MAX_FILE_NAME], name_copy2[MAX_FILE_NAME];
    type pType;
    union Data pdata;

    strcpy(name_copy1, name1);
    strcpy(name_copy2, name2);


    split_parent_child_from_path(name_copy1, &parent_name1, &child_name1);
    split_parent_child_from_path(name_copy2, &parent_name2, &child_name2);

    if (strcmp(parent_name1,parent_name2) < 0){;
        char *v,*b;
        v = strstr(parent_name2, name1);
		b = strstr(parent_name2, parent_name1);
        if(v != NULL){
			printf("failed to move %s, invalid operation.\n",name1);
			return ERROR;
        }
		if(b != NULL){
			parent_inumber1 = lookup(parent_name1, MOVE, vetorlocks, counter); 
			parent_inumber2 = lookup(parent_name2, MOVE, vetorlocks, counter);/*-----------------------------------------*/
		}
        else{
            parent_inumber1 = lookup(parent_name1, MOVE, vetorlocks, counter); 
			parent_inumber2 = lookup(parent_name2, MOVE, vetorlocks, counter);
        }
    }
    else if(strcmp(parent_name2,parent_name1) == 0){
        parent_inumber1 = lookup(parent_name1, MOVE, vetorlocks, counter);
        parent_inumber2 = parent_inumber1;
    }
    else{
        char *v = malloc(sizeof(char)*MAX_FILE_NAME);
        v = strstr(parent_name1, parent_name2);
        if(v != NULL){
            parent_inumber2 = lookup(parent_name2, MOVE, vetorlocks, counter);
			parent_inumber1 = lookup(parent_name1, MOVE, vetorlocks, counter); /*----------------------------------*/
        }
        else{
            parent_inumber2 = lookup(parent_name2, MOVE, vetorlocks, counter);
            parent_inumber1 = lookup(parent_name1, MOVE, vetorlocks, counter);
        }
    }

    if (parent_inumber1 == FAIL) {
        printf("failed to move %s, invalid parent dir %s\n",
                child_name1, parent_name1);
        openlocks(vetorlocks,counter);
        return FAIL;
    }

    inode_get(parent_inumber1, &pType, &pdata);

    if(pType != T_DIRECTORY) {
        printf("failed to move %s, parent %s is not a dir\n",
                child_name1, parent_name1);
        openlocks(vetorlocks,counter);
        return FAIL;
    }

    child_inumber1 = lookup_sub_node(child_name1, pdata.dirEntries);

    if (child_inumber1 == FAIL) {
        printf("could not move %s, does not exist in dir %s\n",
               child_name1, parent_name1);
        openlocks(vetorlocks,counter);
        return FAIL;
    }


    if (parent_inumber2 == FAIL) {
        printf("failed to move %s, invalid parent dir %s\n",
                child_name1, parent_name2);
        openlocks(vetorlocks,counter);
        return FAIL;
    }
    inode_get(parent_inumber2, &pType, &pdata);

    if(pType != T_DIRECTORY) {
        printf("failed to move %s, parent %s is not a dir\n",
                child_name1, parent_name2);
        openlocks(vetorlocks,counter);
        return FAIL;
    }

    if (lookup_sub_node(child_name2, pdata.dirEntries) != FAIL) {
        printf("failed to move %s, already exists in dir %s\n",
               child_name2, parent_name2);
        openlocks(vetorlocks,counter);
        return FAIL;
    }

    if (dir_reset_entry(parent_inumber1, child_inumber1) == FAIL) {
        printf("failed to delete %s from dir %s\n",
               child_name1, parent_name1);
        openlocks(vetorlocks,counter);
        return FAIL;
    }

    if (dir_add_entry(parent_inumber2, child_inumber1, child_name2) == FAIL) {
        printf("could not add entry %s in dir %s\n",
               child_name2, parent_name2);
        openlocks(vetorlocks,counter);
        return FAIL;
    }

    openlocks(vetorlocks, counter);
    return SUCCESS;
}

/*
 * Lookup for a given path.
 * Input:
 *  - name: path of node
 * Returns:
 *  inumber: identifier of the i-node, if found
 *     FAIL: otherwise
 */

int lookup(char *name, int tipo, int vetorlocks[], int *counter) {

	char full_path[MAX_FILE_NAME];
	char delim[] = "/";
	char *saveptr;

	strcpy(full_path, name);

	/* start at root node */
	int current_inumber = FS_ROOT;

	/* use for copy */
	type nType;
	union Data data;

	char *path = strtok_r(full_path, delim, &saveptr);
	if(!(isInVector(current_inumber, vetorlocks, counter))){
		if(path == NULL){
			closelocks(WRITE, &inode_table[current_inumber].lock, vetorlocks, current_inumber, counter);
		}
		else{
			closelocks(READ, &inode_table[current_inumber].lock, vetorlocks, current_inumber, counter);
		}
	}	

	/* get root inode data */
	inode_get(current_inumber, &nType, &data);

	/* search for all sub nodes */

	while (path != NULL && (current_inumber = lookup_sub_node(path, data.dirEntries)) != FAIL) {
		path = strtok_r(NULL, delim, &saveptr);
		if(!(isInVector(current_inumber, vetorlocks, counter))){
			if (path != NULL){
				closelocks(READ,&inode_table[current_inumber].lock, vetorlocks, current_inumber, counter);
			}
			else if(path == NULL){
				if(tipo == LER){
					closelocks(READ,&inode_table[current_inumber].lock, vetorlocks, current_inumber, counter);
				}	
				else{
					closelocks(WRITE,&inode_table[current_inumber].lock, vetorlocks, current_inumber, counter);
				}
			}
		}
		inode_get(current_inumber, &nType, &data);
	}
	if (tipo == LER){
		openlocks(vetorlocks, counter);
	}	
	return current_inumber;
}


/*
 * Prints tecnicofs tree.
 * Input:
 *  - fp: pointer to output file
 */
void print_tecnicofs_tree(FILE *fp){
	inode_print_tree(fp, FS_ROOT, "");
}