#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <ctype.h>
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

void applyCommands(FILE *outputf){
    phread_mutex_lock(lock);
    while (numberCommands > 0){
        const char* command = removeCommand();
        if(numberCommands == 1){
        }
        else{
            pthread_mutex_unlock(lock);
        }
        if (command == NULL){
            continue;
        }

        char token, type;
        char name[MAX_INPUT_SIZE];
        int numTokens = sscanf(command, "%c %s %c", &token, name, &type);
        if (numTokens < 2) {
            fputs("Error: invalid command in Queue\n", file);
            exit(EXIT_FAILURE);
        }

        int searchResult;
        switch (token) {
            case 'c':
                switch (type) {
                    case 'f':
                        pthread_mutex_lock(lockswitch);
                        fprintf(file,"Create file: %s\n", name);
                        create(name, T_FILE);
                        p_thread_mutex_unlock(lockswitch);
                        break;
                    case 'd':
                        pthread_mutex_lock(lockswitch);
                        fprintf(file,"Create directory: %s\n", name);
                        create(name, T_DIRECTORY);
                        p_thread_mutex_unlock(lockswitch);
                        break;
                    default:
                        fprintf(file, "Error: invalid node type\n");
                        exit(EXIT_FAILURE);
                }
                break;
            case 'l':
                pthread_mutex_lock(lockswitch);
                searchResult = lookup(name);
                p_thread_mutex_unlock(lockswitch);
                if (searchResult >= 0)
                    fprintf(file, "Search: %s found\n", name);
                else
                    fprintf(file, "Search: %s not found\n", name);
                break;
            case 'd':
                pthread_mutex_lock(lockswitch);
                fprintf(file, "Delete: %s\n", name);
                delete(name);
                p_thread_mutex_unlock(lockswitch);
                break;
            default: { /* error */
                fprintf(file, "Error: command to apply\n");
                exit(EXIT_FAILURE);
            }
        }
        if(numberCommands == 1)
            pthread_mutex_unlock(lock);
    }
}

int main(int argc,char* argv[]) {
    /* init filesystem */
    char *inputf = argv[1];
    char *outputf = argv[2];
    int numthreads=argv[3];
    pthread_mutex_t lock;
    pthread_mutex_t lockswitch;
    pthread_mutex_init(&lock,NULL);
    pthread_mutex_init(&lockswitch,NULL);
    init_fs();

    /* process input and print tree */
    processInput(inputf);
    /* */
    pthread_create(...);
    pthread_create(...);
    pthread_create(...);
    pthread_create(...);
    FILE *file = fopen("outputf","w");
    applyCommands(outputf);
    fclose(file);
    print_tecnicofs_tree(outputf);

    /* release allocated memory */
    destroy_fs();
    exit(EXIT_SUCCESS);
}
