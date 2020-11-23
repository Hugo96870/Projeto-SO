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
#include <errno.h>
#include "fs/operations.h"

#define INDIM 30
#define OUTDIM 512
#define MAX_INPUT_SIZE 100

int sockfd;
/*
struct timespec start, finish;
double timeSpent;
*/

int setSockAddrUn(char *path, struct sockaddr_un *addr) {

  if (addr == NULL)
    return 0;

  bzero((char *)addr, sizeof(struct sockaddr_un));
  addr->sun_family = AF_UNIX;
  strcpy(addr->sun_path, path);

  return SUN_LEN(addr);
}

void errorParse(){
    fprintf(stderr, "Error: command invalid\n");
    exit(EXIT_FAILURE);
}

/* Apply the commands that where processed from the file to inputCommands in the previous \
 * function (processInput).
 */
void* applyCommands(){
    int *locksVector = malloc(sizeof(int) * 50);
    int *counter = malloc(sizeof(int));

    while(1){ /* The loop stops when there are no more commands to be processed and done */
        struct sockaddr_un client_addr;
        char in_buffer[MAX_INPUT_SIZE], out_buffer[MAX_INPUT_SIZE];
        int c;
        socklen_t addrlen;

        addrlen = sizeof(struct sockaddr_un);

        c = recvfrom(sockfd, in_buffer, sizeof(in_buffer)-1, 0, (struct sockaddr *)&client_addr, &addrlen);
        if (c <= 0)
            continue;
        in_buffer[c]='\0';

        printf("Recebeu mensagem de %s\n", client_addr.sun_path);

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
                        c = create(name, T_FILE);
                        out_buffer[0] = c - '0';
                        out_buffer[1] = '\0';
                        printf("Server: %s \n", client_addr.sun_path);
                        int err = sendto(sockfd, out_buffer, 2, 0, (struct sockaddr *)&client_addr, addrlen);
                        if(err < 0){
                            printf("Erro: %d\n",errno);
                        }
                        printf("enviei\n");
                        break;
                    case 'd':
                        printf("Create directory: %s\n", name);
                        c = create(name, T_DIRECTORY);
                        out_buffer[0] = c - '0';
                        out_buffer[1] = '\0';
                        sendto(sockfd, out_buffer, 2, 0, (struct sockaddr *)&client_addr, addrlen);
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
                out_buffer[0] = searchResult - '0';
                out_buffer[1] = '\0';
                sendto(sockfd, out_buffer, 2, 0, (struct sockaddr *)&client_addr, addrlen);
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
                c = move(name, type);
                out_buffer[0] = c - '0';
                out_buffer[1] = '\0';
                sendto(sockfd, out_buffer, 2, 0, (struct sockaddr *)&client_addr, addrlen);
                break;
            case 'd':
                if(numTokens != 2)
                    errorParse();
                printf("Delete: %s\n", name);
                c = delete(name);
                out_buffer[0] = c - '0';
                out_buffer[1] = '\0';
                sendto(sockfd, out_buffer, 2, 0, (struct sockaddr *)&client_addr, addrlen);
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
void startThreads(int nrT){
    int i;
    pthread_t *tid = malloc(sizeof(pthread_t)*nrT);

    for(i = 0; i < nrT; i++){ /* Initializes the threads that will apply the commands. */
        if ((pthread_create(&tid[i], 0, applyCommands, NULL) != 0)){ 
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
    struct sockaddr_un server_addr;
    socklen_t addrlen;

    if ((sockfd = socket(AF_UNIX, SOCK_DGRAM, 0)) < 0) {
        perror("server: can't open socket");
        exit(EXIT_FAILURE);
    }

    unlink(name);

    addrlen = setSockAddrUn (name, &server_addr);
    if (bind(sockfd, (struct sockaddr *) &server_addr, addrlen) < 0) {
        perror("server: bind error");
        exit(EXIT_FAILURE);
    }
    return sockfd;
}

int main(int argc, char* argv[]) {


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
    startThreads(atoi(argv[1]));

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
    destroy_fs();
    exit(EXIT_SUCCESS);
}