#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <math.h>
#include <pthread.h>
#include "fs/operations.h"

#define MAX_COMMANDS 10
#define MAX_INPUT_SIZE 100

pthread_mutex_t lockm;
pthread_mutex_t state;
pthread_mutex_t lockwhile;
pthread_mutex_t lockVect;
pthread_cond_t write;
pthread_cond_t read;

struct timespec start, finish;
double timeSpent;
int count=0;
int readptr=0;
int writeptr=0;

char inputCommands[MAX_COMMANDS][MAX_INPUT_SIZE];
int readState = 1;


void errorParse(){
    fprintf(stderr, "Error: command invalid\n");
    exit(EXIT_FAILURE);
}

void* processInput(void* arg){
    FILE *inputf = (FILE*)arg;
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
        if (pthread_mutex_lock(&lockVect) != 0){
            printf("Error: Failed to close lock.\n");
			exit(EXIT_FAILURE);
        }
        while(count==MAX_COMMANDS){
            pthread_cond_wait(&write,&lockVect);
        }
        switch (token) {
            case 'c':
                if(numTokens != 3)
                    errorParse();
                break;
            
            case 'l':
                if(numTokens != 2)
                    errorParse();
                break;
            
            case 'd':
                if(numTokens != 2)
                    errorParse();       
                break;
            case 'm':
                if(numTokens != 2)
                    errorParse();
                break;

            case '#':
                break;
            
            default: { /* error */
                errorParse();
            }
        }
        if(token!='#'){ 
            strcpy(inputCommands[writeptr], line);
            writeptr++;
            if(writeptr==MAX_COMMANDS)
                writeptr=0;
            count++; 
        }
        pthread_cond_signal(&read);
        pthread_mutex_unlock(&lockVect);
    }
    if (pthread_mutex_lock(&state) != 0){
        printf("Error: Failed to close lock.\n");
        exit(EXIT_FAILURE);
    }
    readState = 0;
    pthread_cond_broadcast(&read);
    if (pthread_mutex_unlock(&state) != 0){
        printf("Error: Failed to open lock.\n");
        exit(EXIT_FAILURE);
    }
    return 0;
}

void* applyCommands(){
    int *vetorlocks=malloc(sizeof(int)*50);
    int *counter=malloc(sizeof(int));
    if (pthread_mutex_lock(&lockwhile) != 0){
        printf("Error: Failed to close lock.\n");
        exit(EXIT_FAILURE);
    }
    while(count > 0 || readState == 1){
        if (pthread_mutex_unlock(&lockwhile) != 0){
            printf("Error: Failed to open lock.\n");
            exit(EXIT_FAILURE);
        }
        const char* command;
        if (pthread_mutex_lock(&lockVect) != 0){
            printf("Error: Failed to close lock.\n");
			exit(EXIT_FAILURE);
        }
        while(count==0 && readState==1){
            pthread_cond_wait(&read,&lockVect);
        }
        command = inputCommands[readptr];
        /* Secção critica ????????????????????????? */
        readptr++;
        if(readptr == MAX_COMMANDS)
            readptr = 0;
        count--;
        pthread_cond_signal(&write);
        pthread_mutex_unlock(&lockVect);

        if(command==NULL){
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
            case 'm':
                printf("Move: %s\n", name);
                move();
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
        if (pthread_mutex_lock(&lockwhile) != 0){
            printf("Error: Failed to close lock.\n");
            exit(EXIT_FAILURE);
        }
    }
    if (pthread_mutex_unlock(&lockwhile) != 0){
        printf("Error: Failed to open lock.\n");
        exit(EXIT_FAILURE);
    }
    return 0;
}

void startThreadRead(FILE *inputf){
    pthread_t *tid = malloc(sizeof(pthread_t));
    if ((pthread_create(tid,0,processInput,inputf) != 0)){
        printf("Cannot create thread.\n");
        exit(EXIT_FAILURE);
    }
    if ((pthread_join(*tid,NULL) != 0)){
        printf("Cannot join thread.\n");
        exit(EXIT_FAILURE);
    }
    free(tid);
}

/* Initializes threads.
 * Input:
 *  - nrT: Number of threads to initialize.
 */
void startThreads(int nrT, FILE *inputf){
    int i;
    pthread_t *tid=malloc(sizeof(pthread_t)*(nrT+1));

    if ((pthread_create(&tid[0],0,processInput,inputf) != 0)){
        printf("Cannot create thread.\n");
        exit(EXIT_FAILURE);
    }

    for(i=1;i<=nrT;i++){
        if ((pthread_create(&tid[i],0,applyCommands,NULL) != 0)){
            printf("Cannot create thread.\n");
            exit(EXIT_FAILURE);
        }
    }
    for(i=0;i<=nrT;i++){
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

    pthread_mutex_init(&lockVect,NULL);
    pthread_mutex_init(&lockm,NULL);
    pthread_mutex_init(&state,NULL);
    pthread_mutex_init(&lockwhile,NULL);
    pthread_cond_init(&read,NULL);
    pthread_cond_init(&write,NULL);

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

    if(clock_gettime(CLOCK_REALTIME, &start)!=0){
        printf("Error: cant open clock\n");
        exit(EXIT_FAILURE);
    }
    /* process input and print tree */
    startThreads(atoi(argv[3]),inputf);
    fclose(inputf);
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
    pthread_cond_destroy(&read);
    pthread_cond_destroy(&write);
    pthread_mutex_destroy(&state);
    pthread_mutex_destroy(&lockm);
    pthread_mutex_destroy(&lockwhile);
    pthread_mutex_destroy(&lockVect);
    destroy_fs();
    exit(EXIT_SUCCESS);
}