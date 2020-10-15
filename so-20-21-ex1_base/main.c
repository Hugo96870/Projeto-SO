#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <math.h>
#include <pthread.h>
#include "fs/operations.h"

#define MAX_COMMANDS 150000
#define MAX_INPUT_SIZE 100

pthread_mutex_t lockmCom;
pthread_mutex_t lockm;
pthread_rwlock_t lockrw;
char p[6];

int numberThreads = 0;

char inputCommands[MAX_COMMANDS][MAX_INPUT_SIZE];
int numberCommands = 0;
int headQueue = 0;

int insertCommand(char* data) {
    if(numberCommands != MAX_COMMANDS){
        strcpy(inputCommands[numberCommands++], data);
        return 1;
    }
    return 0;
}

char* removeCommand() {
    if(numberCommands > 0){
        numberCommands--;
        return inputCommands[headQueue++];  
    }
    return NULL;
}

void errorParse(){
    fprintf(stderr, "Error: command invalid\n");
    exit(EXIT_FAILURE);
}

void processInput(FILE *inputf){
    char line[MAX_INPUT_SIZE];

    /*if (file == NULL){
    }*/

    /* break loop with ^Z or ^D */
    while (fgets(line,sizeof(line)/sizeof(char),inputf)) {
        char token, type;
        char name[MAX_INPUT_SIZE];

        int numTokens = sscanf(line, "%c %s %c", &token, name, &type);

        /* perform minimal validation */
        if (numTokens < 1) {
            continue;
        }
        switch (token) {
            case 'c':
                if(numTokens != 3)
                    errorParse();
                if(insertCommand(line))
                    break;
                return;
            
            case 'l':
                if(numTokens != 2)
                    errorParse();
                if(insertCommand(line))
                    break;
                return;
            
            case 'd':
                if(numTokens != 2)
                    errorParse();
                if(insertCommand(line))
                    break;
                return;
            
            case '#':
                break;
            
            default: { /* error */
                errorParse();
            }
        }
    }
}

void* applyCommands(){
    int tid;
    tid=pthread_self();

    pthread_mutex_lock(&lockmCom);
    while(numberCommands>0){
        const char* command = removeCommand();
        pthread_mutex_unlock(&lockmCom);
        if(command==NULL){
            pthread_mutex_lock(&lockmCom);
            continue;
        }
        char token, type;
        char name[MAX_INPUT_SIZE];
        int numTokens = sscanf(command, "%c %s %c", &token, name, &type);
        if (numTokens < 2) {
            fprintf(stderr, "Error: invalid command in Queue\n");
            exit(EXIT_FAILURE);
        }
        int searchResult;
        switch (token) {
            case 'c':
                switch (type) {
                    case 'f':
                        printf("Create file: %s\n", name);
                        printf("tarefa : %d encontrei\n",tid);
                        closelocks("wr");
                        create(name, T_FILE);
                        openlocks();
                        break;
                    case 'd':
                        printf("Create directory: %s\n", name);
                        printf("tarefa : %d encontrei\n",tid);
                        closelocks("wr");
                        create(name, T_DIRECTORY);
                        openlocks();
                        break;
                    default:
                        fprintf(stderr, "Error: invalid node type\n");
                        exit(EXIT_FAILURE);
                }
                break;
            case 'l':
                closelocks("rd");
                searchResult = lookup(name);
                openlocks();
                if (searchResult >= 0){
                    printf("Search: %s found\n", name);
                }
                else{
                    printf("Search: %s not found\n", name);
                }
                break;
            case 'd':
                printf("Delete: %s\n", name);
                closelocks("rw");
                delete(name);
                openlocks();
                break;
            default: { /* error */
                fprintf(stderr, "Error: command to apply\n");
                exit(EXIT_FAILURE);
            }
        }
        pthread_mutex_lock(&lockmCom);
    }
    pthread_mutex_unlock(&lockmCom);
    return 0;
}

void startThreads(int nrT){
    int i;
    pthread_t *tid=malloc(sizeof(pthread_t)*nrT);

    for(i=0;i<nrT;i++){
        pthread_create(&tid[i],0,applyCommands,NULL);
    }
    for(i=0;i<nrT;i++){
        pthread_join(tid[i],NULL);
    }
    free(tid);
}

int main(int argc,char* argv[]) {
    /* init filesystem */
    FILE *inputf;
    FILE *outputf;

    if(strcmp(argv[4],"mutex")==0){
        pthread_mutex_init(&lockm,NULL);
        pthread_mutex_init(&lockmCom,NULL);
        strcpy(p,argv[4]);
    }
    else if(strcmp(argv[4],"rwlock")==0){
        pthread_rwlock_init(&lockrw,NULL);
        pthread_mutex_init(&lockmCom,NULL);
        strcpy(p,argv[4]);
    }
    else if(strcmp(argv[4],"nosync")==0){
        pthread_mutex_init(&lockmCom,NULL);
        strcpy(p,argv[4]);
    }

    init_fs();
    inputf = fopen(argv[1],"r");
    /* process input and print tree */
    processInput(inputf);
    fclose(inputf);
    startThreads(atoi(argv[3]));
    outputf = fopen(argv[2],"w");
    print_tecnicofs_tree(outputf);
    fclose(outputf);
    /* release allocated memory */
    destroy_fs();
    exit(EXIT_SUCCESS);
}