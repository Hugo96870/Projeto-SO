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

pthread_mutex_t lockm;
pthread_mutex_t lockmCom;

struct timespec start, finish;
double timeSpent;
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
    int *vetorlocks=malloc(sizeof(int)*50);
    int *counter=malloc(sizeof(int));

    if (pthread_mutex_lock(&lockmCom) != 0){
        printf("Error: Failed to close lock.\n");
		exit(EXIT_FAILURE);
    }
    while(numberCommands>0){
        const char* command = removeCommand();
        if (pthread_mutex_unlock(&lockmCom) != 0){
            printf("Error: Failed to open lock.\n");
			exit(EXIT_FAILURE);
        }
        if(command==NULL){
            if (pthread_mutex_lock(&lockmCom) != 0){
                printf("Error: Failed to close lock.\n");
			    exit(EXIT_FAILURE);
            }
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
                        create(name, T_FILE);
                        break;
                    case 'd':
                        printf("Create directory: %s\n", name);
                        create(name, T_DIRECTORY);
                        break;
                    default:
                        fprintf(stderr, "Error: invalid node type\n");
                        exit(EXIT_FAILURE);
                }
                break;
            case 'l':
            *counter=0;
                searchResult = lookup(name,1,vetorlocks,counter);
                if (searchResult >= 0){
                    printf("Search: %s found\n", name);
                }
                else{
                    printf("Search: %s not found\n", name);
                }
                break;
            case 'd':
                printf("Delete: %s\n", name);
                delete(name);
                break;
            default: { /* error */
                fprintf(stderr, "Error: command to apply\n");
                exit(EXIT_FAILURE);
            }   
        }
        if (pthread_mutex_lock(&lockmCom) != 0){
            printf("Error: Failed to close lock.\n");
			exit(EXIT_FAILURE);
        }
    }
    if (pthread_mutex_unlock(&lockmCom) != 0){
        printf("Error: Failed to open lock.\n");
		exit(EXIT_FAILURE);
    } 
    return 0;
}

/* Initializes threads.
 * Input:
 *  - nrT: Number of threads to initialize.
 */
void startThreads(int nrT){
    int i;
    pthread_t *tid=malloc(sizeof(pthread_t)*nrT);

    for(i=0;i<nrT;i++){
        if ((pthread_create(&tid[i],0,applyCommands,NULL) != 0)){
            printf("Cannot create thread.\n");
            exit(EXIT_FAILURE);
        }
    }
    for(i=0;i<nrT;i++){
        if ((pthread_join(tid[i],NULL) != 0)){
            printf("Cannot join thread.\n");
            exit(EXIT_FAILURE);
        }
    }
    free(tid);
}

int main(int argc,char* argv[]) {
    FILE *inputf;
    FILE *outputf;

    pthread_mutex_init(&lockmCom,NULL);
    pthread_mutex_init(&lockm,NULL);

    /* Verify the input */
    if(argc != 4 || atoi(argv[3]) < 1){
        printf("Error: Invalid input");
        exit(EXIT_FAILURE);
    }

    /* init filesystem */
    init_fs();
    if ((inputf = fopen(argv[1],"r")) == NULL){
        printf("Error: Cannot open file.\n");
        exit(EXIT_FAILURE);
    };

    /* process input and print tree */
    processInput(inputf);
    fclose(inputf);
    if(clock_gettime(CLOCK_REALTIME, &start)!=0){
        printf("Error: cant open clock\n");
        exit(EXIT_FAILURE);
    }
    startThreads(atoi(argv[3]));
    if ((outputf = fopen(argv[2],"w")) == NULL ){
        printf("Error: Cannot open file.\n");
        exit(EXIT_FAILURE);
    }
    if(clock_gettime(CLOCK_REALTIME, &finish)!=0){
        printf("Error: Cant close clock\n");
        exit(EXIT_FAILURE);
    }
    timeSpent = (finish.tv_sec - start.tv_sec);
    timeSpent += (finish.tv_nsec - start.tv_nsec) / 1000000000.0;
    printf("TecnicoFS completed in %.4f seconds.\n", timeSpent);
    print_tecnicofs_tree(outputf);
    fclose(outputf);
    /* release allocated memory */
    pthread_mutex_destroy(&lockm);
    pthread_mutex_destroy(&lockmCom);
    destroy_fs();
    exit(EXIT_SUCCESS);
}