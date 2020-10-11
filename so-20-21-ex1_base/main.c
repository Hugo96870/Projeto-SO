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
    FILE *file = fopen("inputf", "r");

    /*if (file == NULL){
    }*/

    /* break loop with ^Z or ^D */
    while (fgets(line,sizeof(line)/sizeof(char),file)) {
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
    fclose(file);
}

void* applyCommands(){
    
    while (numberCommands > 0){
        const char* command = removeCommand();
        if (command == NULL){
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
                searchResult = lookup(name);
                if (searchResult >= 0)
                    printf("Search: %s found\n", name);
                else
                    printf("Search: %s not found\n", name);
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
    }
    return 0;
}

void startThreads(int nrT,clock_t begin){
    int i;
    pthread_t tid[nrT];
    begin = clock();
    for(i = 0; i < nrT; i++){
        pthread_create(&tid[i],0,applyCommands,NULL);
    }
    for(i = 0; i < nrT; i++){
        pthread_join(tid[i],NULL);
    }
}

int main(int argc,char* argv[]) {
    /* init filesystem */
    double timeSpent;
    clock_t begin;
    FILE *inputf;
    FILE *outputf;
    inputf = fopen(argv[1],"r");
    outputf = fopen(argv[2],"w");
    init_fs();

    /* process input and print tree */
    processInput(inputf);

    startThreads(atoi(argv[3]),begin);
    clock_t end=clock();
    timeSpent=(double)(end-begin) / CLOCKS_PER_SEC;
    timeSpent = floor(10000*timeSpent)/10000;
    printf("TecnicoFS completed in %f seconds.",timeSpent);
    print_tecnicofs_tree(outputf);

    /* release allocated memory */
    destroy_fs();
    exit(EXIT_SUCCESS);
}