/* Grupo 11 - Bernardo Castiço ist196845
 *          - Hugo Rita ist196870
 */

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

pthread_mutex_t state;
pthread_mutex_t lockwhile;
pthread_mutex_t lockVect;
pthread_cond_t write;
pthread_cond_t read;

struct timespec start, finish;
double timeSpent;
int count = 0;
int readptr = 0;
int writeptr = 0;

char inputCommands[MAX_COMMANDS][MAX_INPUT_SIZE];
int readState = 1; /* flag that indicates if there are commands to read */


void errorParse(){
    fprintf(stderr, "Error: command invalid\n");
    exit(EXIT_FAILURE);
}

/* Process the input. Receives the input file and put the commands in inputCommads \
 * so they can be acessed by threads in a circular way.
 * Input:
 *  - arg: Input file where the commands to be processed are.
 */

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
        while(count == MAX_COMMANDS){
            if (pthread_cond_wait(&write,&lockVect) != 0){
                printf("Error: Failed to wait.\n");
			    exit(EXIT_FAILURE);
            }
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
                if(numTokens != 3)
                    errorParse();
                break;

            case '#':
                break;
            
            default: { /* error */
                errorParse();
            }
        }
        if(token != '#'){   /* The file line is a command to execute */
            strcpy(inputCommands[writeptr], line);
            writeptr++;
            if(writeptr == MAX_COMMANDS){
                writeptr = 0;
            }
            count++; 
        }
        if (pthread_cond_signal(&read) != 0){
            printf("Error: Failed to signal.\n");
            exit(EXIT_FAILURE);
        }
        
        if (pthread_mutex_unlock(&lockVect) != 0){
            printf("Error: Failed to open lock.\n");
            exit(EXIT_FAILURE);
        }
    }
    if (pthread_mutex_lock(&state) != 0){
        printf("Error: Failed to close lock.\n");
        exit(EXIT_FAILURE);
    } 
    readState = 0;  /* The flag turns 0 when there are no more commands to process from file */

    if (pthread_cond_broadcast(&read) != 0){
        printf("Error: Failed to broadcast.\n");
        exit(EXIT_FAILURE);
    }

    if (pthread_mutex_unlock(&state) != 0){
        printf("Error: Failed to open lock.\n");
        exit(EXIT_FAILURE);
    }
    return 0;
}


/* Apply the commands that where processed from the file to inputCommands in the previous \
 * function (processInput).
 */
void* applyCommands(){
    int *locksVector = malloc(sizeof(int) * 50);
    int *counter = malloc(sizeof(int));

    if (pthread_mutex_lock(&lockwhile) != 0){
        printf("Error: Failed to close lock.\n");
        exit(EXIT_FAILURE);
    }
    
    while(count > 0 || readState == 1){ /* The loop stops when there are no more commands to be processed and done */
        if (pthread_mutex_unlock(&lockwhile) != 0){
            printf("Error: Failed to open lock.\n");
            exit(EXIT_FAILURE);
        }
        const char* command;
        if (pthread_mutex_lock(&lockVect) != 0){
            printf("Error: Failed to close lock.\n");
			exit(EXIT_FAILURE);
        }
        while(count==0 && readState==1){ /* Waits until there is a command to be processed and done */
            if (pthread_cond_wait(&read,&lockVect) != 0){
                printf("Error: Failed to wait.\n");
			    exit(EXIT_FAILURE);
            }
        }
        command = inputCommands[readptr];

        readptr++;
        if(readptr == MAX_COMMANDS){
            readptr = 0;
        } 
        count--;
        
        if (count < 0){
            if (pthread_mutex_unlock(&lockVect) != 0){
                printf("Error: Failed to open lock.\n");
                exit(EXIT_FAILURE);
            }
            break;
        }

        if (pthread_cond_signal(&write) != 0){
            printf("Error: Failed to signal.\n");
			exit(EXIT_FAILURE);
        }
        if (pthread_mutex_unlock(&lockVect) != 0){
            printf("Error: Failed to open lock.\n");
			exit(EXIT_FAILURE);
        }

        if(command == NULL){
            continue;
        }

        char token;
        char name[MAX_INPUT_SIZE], type[MAX_INPUT_SIZE];
        int numTokens = sscanf(command, "%c %s %s", &token, name, type);
        if (numTokens < 2) {
            fprintf(stderr, "Error: invalid command in Queue\n");
            exit(EXIT_FAILURE);
        }
        int searchResult;
        switch (token) {
            case 'c':
                switch (type[0]) {
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
                searchResult = lookup(name, 0, locksVector, counter);
                if (searchResult >= 0){
                    printf("Search: %s found\n", name);
                }
                else{
                    printf("Search: %s not found\n", name);
                }
                break;
            case 'm':
                printf("Move: %s %s\n", name, type);
                move(name, type);
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

/* Initializes threads.
 * Input:
 *  - nrT: Number of threads to initialize.
 *  - inputf: Input file.
 */
void startThreads(int nrT, FILE *inputf){
    int i;
    pthread_t *tid = malloc(sizeof(pthread_t)*(nrT+1));

    if ((pthread_create(&tid[0], 0, processInput, inputf) != 0)){ /*Initializes the thread that will process the commands.*/
        printf("Cannot create thread.\n");
        exit(EXIT_FAILURE);
    }

    for(i = 1; i <= nrT; i++){ /* Initializes the threads that will apply the commands. */
        if ((pthread_create(&tid[i], 0, applyCommands, NULL) != 0)){ 
            printf("Cannot create thread.\n");
            exit(EXIT_FAILURE);
        }
    }
    for(i = 0;i <= nrT; i++){
        if ((pthread_join(tid[i], NULL) != 0)){
            printf("Cannot join thread.\n");
            exit(EXIT_FAILURE);
        }
    }
    free(tid);
}

int main(int argc, char* argv[]) {
    FILE *inputf;
    FILE *outputf;

    /* init mutex and cond variables */
    pthread_mutex_init(&lockVect,NULL);
    pthread_mutex_init(&state,NULL);
    pthread_mutex_init(&lockwhile,NULL);
    pthread_cond_init(&read,NULL);
    pthread_cond_init(&write,NULL);

    /* verify the input */
    if(argc != 4 || atoi(argv[3]) < 1){
        printf("Error: Invalid input");
        exit(EXIT_FAILURE);
    }

    /* init filesystem */
    init_fs();
    if ((inputf = fopen(argv[1], "r")) == NULL){
        printf("Error: Cannot open file.\n");
        exit(EXIT_FAILURE);
    }

    /* init clock */
    if(clock_gettime(CLOCK_REALTIME, &start)!=0){
        printf("Error: cant open clock\n");
        exit(EXIT_FAILURE);
    }

    /* process input */
    startThreads(atoi(argv[3]), inputf);
    fclose(inputf);
    if ((outputf = fopen(argv[2], "w")) == NULL ){
        printf("Error: Cannot open file.\n");
        exit(EXIT_FAILURE);
    }

    /* finish clock */
    if(clock_gettime(CLOCK_REALTIME, &finish)!=0){
        printf("Error: Cant close clock\n");
        exit(EXIT_FAILURE);
    }
    timeSpent = (finish.tv_sec - start.tv_sec);
    timeSpent += (finish.tv_nsec - start.tv_nsec) / 1000000000.0;
    printf("TecnicoFS completed in %.4f seconds.\n", timeSpent);
    
    /* print tree */
    print_tecnicofs_tree(outputf);
    fclose(outputf);

    /* release allocated memory */
    pthread_cond_destroy(&read);
    pthread_cond_destroy(&write);
    pthread_mutex_destroy(&state);
    pthread_mutex_destroy(&lockwhile);
    pthread_mutex_destroy(&lockVect);

    destroy_fs();
    exit(EXIT_SUCCESS);
}