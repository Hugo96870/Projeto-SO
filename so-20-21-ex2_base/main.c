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
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <strings.h>
#include <sys/uio.h>
#include <unistd.h>
#include "fs/operations.h"

#define INDIM 30
#define OUTDIM 512
#define MAX_INPUT_SIZE 100

pthread_mutex_t lockVect;
/*
struct timespec start, finish;
double timeSpent;
*/


void errorParse(){
    fprintf(stderr, "Error: command invalid\n");
    exit(EXIT_FAILURE);
}

/* Apply the commands that where processed from the file to inputCommands in the previous \
 * function (processInput).
 */
void* applyCommands(void *arg){
    int *locksVector = malloc(sizeof(int) * 50);
    int *counter = malloc(sizeof(int));
    int sockfd = (int*)arg;

    while(1){ /* The loop stops when there are no more commands to be processed and done */
        struct sockaddr_un client_addr;
        char in_buffer[INDIM], out_buffer[OUTDIM];
        int c;
        socklen_t addrlen;

        if (pthread_mutex_lock(&lockVect) != 0){
            printf("Error: Failed to close lock.\n");
			exit(EXIT_FAILURE);
        }

        addrlen = sizeof(struct sockaddr_un);
        c = recvfrom(sockfd, in_buffer, sizeof(in_buffer)-1, 0, (struct sockaddr *)&client_addr, &addrlen);
        if (c <= 0)
            continue;
        in_buffer[c]='\0';

        printf("Recebeu mensagem de %s\n", client_addr.sun_path);
        
        if (pthread_mutex_unlock(&lockVect) != 0){
            printf("Error: Failed to open lock.\n");
			exit(EXIT_FAILURE);
        }

        if(in_buffer == NULL){
            continue;
        }

        char token;
        char name[MAX_INPUT_SIZE], type[MAX_INPUT_SIZE];
        int numTokens = sscanf(in_buffer, "%c %s %s", &token, name, type);
        if (numTokens < 2) {
            fprintf(stderr, "Error: invalid command in Queue\n");
            exit(EXIT_FAILURE);
        }
        int searchResult;
        switch (token) {
            case 'c':
                if(numTokens != 3)
                    errorParse();
                switch (type[0]) {
                    case 'f':
                        printf("Create file: %s\n", name);
                        create(name, T_FILE);
                        sendto(sockfd, out_buffer, c+1, 0, (struct sockaddr *)&client_addr, addrlen);
                        break;
                    case 'd':
                        printf("Create directory: %s\n", name);
                        create(name, T_DIRECTORY);
                        sendto(sockfd, out_buffer, c+1, 0, (struct sockaddr *)&client_addr, addrlen);
                        break;
                    default:
                        fprintf(stderr, "Error: invalid node type\n");
                        exit(EXIT_FAILURE);
                }
                break;
            case 'l':
                if(numTokens != 2)
                    errorParse();       
                *counter=0;
                searchResult = lookup(name, 0, locksVector, counter);
                sendto(sockfd, out_buffer, c+1, 0, (struct sockaddr *)&client_addr, addrlen);
                if (searchResult >= 0){
                    printf("Search: %s found\n", name);
                }
                else{
                    printf("Search: %s not found\n", name);
                }
                break;
            case 'm':
                if(numTokens != 3)
                    errorParse();       
                printf("Move: %s %s\n", name, type);
                move(name, type);
                sendto(sockfd, out_buffer, c+1, 0, (struct sockaddr *)&client_addr, addrlen);
                break;
            case 'd':
                if(numTokens != 2)
                    errorParse();
                printf("Delete: %s\n", name);
                delete(name);
                c = sprintf(out_buffer, "Ola' %s, que tal vai isso?", in_buffer);
                sendto(sockfd, out_buffer, c+1, 0, (struct sockaddr *)&client_addr, addrlen);
                break;
            default: { /* error */
                fprintf(stderr, "Error: command to apply\n");
                exit(EXIT_FAILURE);
            }   
        }
    }
    return 0;
}

/* Initializes threads.
 * Input:
 *  - nrT: Number of threads to initialize.
 *  - inputf: Input file.
 */
void startThreads(int nrT, int *sockfd){
    int i;
    pthread_t *tid = malloc(sizeof(pthread_t)*nrT);

    for(i = 0; i < nrT; i++){ /* Initializes the threads that will apply the commands. */
        if ((pthread_create(&tid[i], 0, applyCommands, *sockfd) != 0)){ 
            printf("Cannot create thread.\n");
            exit(EXIT_FAILURE);
        }
    }
    for(i = 0;i < nrT; i++){
        if ((pthread_join(tid[i], NULL) != 0)){
            printf("Cannot join thread.\n");
            exit(EXIT_FAILURE);
        }
    }
    free(tid);
}

int createSocket(char* name){
    int sockfd;
    struct sockaddr_un server_addr;
    socklen_t addrlen;

    if ((sockfd = socket(AF_UNIX, SOCK_DGRAM, 0)) < 0) {
        perror("server: can't open socket");
        exit(EXIT_FAILURE);
    }

    addrlen = setSockAddrUn (name, &server_addr);
    if (bind(sockfd, (struct sockaddr *) &server_addr, addrlen) < 0) {
        perror("server: bind error");
        exit(EXIT_FAILURE);
    }
    return sockfd;
}

int main(int argc, char* argv[]) {

    /* init mutex and cond variables */
    pthread_mutex_init(&lockVect,NULL);

    /* verify the input */
    if(argc != 3 || atoi(argv[1]) < 1){
        printf("Error: Invalid input");
        exit(EXIT_FAILURE);
    }

    /* init filesystem */
    init_fs();


    int *sockfd = malloc(sizeof(int));
    *sockfd = createSocket(argv[2]);

    /* init clock 
    if(clock_gettime(CLOCK_REALTIME, &start)!=0){
        printf("Error: cant open clock\n");
        exit(EXIT_FAILURE);
    }*/

    /* process input */
    startThreads(atoi(argv[1]), sockfd);

    /* finish clock 
    if(clock_gettime(CLOCK_REALTIME, &finish)!=0){
        printf("Error: Cant close clock\n");
        exit(EXIT_FAILURE);
    }
    timeSpent = (finish.tv_sec - start.tv_sec);
    timeSpent += (finish.tv_nsec - start.tv_nsec) / 1000000000.0;
    printf("TecnicoFS completed in %.4f seconds.\n", timeSpent);*/
    
    /* print tree 
    print_tecnicofs_tree(outputf);*/

    /* release allocated memory */
    pthread_mutex_destroy(&lockVect);

    destroy_fs();
    exit(EXIT_SUCCESS);
}