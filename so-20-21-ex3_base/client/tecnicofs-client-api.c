#include "tecnicofs-client-api.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <sys/types.h>
#include <strings.h>
#include <sys/uio.h>
#include <sys/stat.h>

#define MAX_INPUT_SIZE 100
#define FAIL 1
#define SUCCESS 0
#define ERROR -1

int sockfd;
socklen_t servlen, clilen;
struct sockaddr_un serv_addr, client_addr;
char buffer[MAX_INPUT_SIZE];
char nameSocket[MAX_INPUT_SIZE];

int setSockAddrUn(char *path, struct sockaddr_un *addr) {

  if (addr == NULL)
    return 0;

  bzero((char *)addr, sizeof(struct sockaddr_un));
  addr->sun_family = AF_UNIX;
  strcpy(addr->sun_path, path);

  return SUN_LEN(addr);
}

/*
 * Sends create command to server and receives a message
 * Input:
 *  - filename: path of to new file or directory.
 *  - nodeType: indentifies if is a file or directory.
 * Returns: SUCCESS OR FAIL
 */
int tfsCreate(char *filename, char nodeType) {

  char c[MAX_INPUT_SIZE];
  int size;

  size = sprintf(c,"c %s %c",filename, nodeType);

  if (sendto(sockfd, c, size, 0, (struct sockaddr *) &serv_addr, servlen) < 0) {
    perror("client: sendto error\n");
    exit(EXIT_FAILURE);
  } 

  if (recvfrom(sockfd, buffer, sizeof(buffer), 0,(struct sockaddr *) &serv_addr, &servlen) < 0) {
    perror("client: recvfrom error\n");
    exit(EXIT_FAILURE);
  }

  if(atoi(buffer) < 0)
    return FAIL;
  else
    return SUCCESS;
  
  exit(EXIT_FAILURE);
}

/*
 * Sends delete command to server and receives a message
 * Input:
 *  - filename: path to the file or directory.
 * Returns: SUCCESS OR ERROR
 */
int tfsDelete(char *path) {

  char c[MAX_INPUT_SIZE];
  int size;

  size = sprintf(c,"d %s",path);

  if (sendto(sockfd, c, size, 0, (struct sockaddr *) &serv_addr, servlen) < 0) {
    perror("client: sendto error");
    exit(EXIT_FAILURE);
  } 

  if (recvfrom(sockfd, buffer, sizeof(buffer), 0,(struct sockaddr *) &serv_addr, &servlen) < 0) {
    perror("client: recvfrom error");
    exit(EXIT_FAILURE);
  }

  if(atoi(buffer) < 0)
    return ERROR;
  else
    return SUCCESS;
  
  exit(EXIT_FAILURE);
}

/*
 * Sends move command to server and receives a message
 * Input:
 *  - from: path to the file or directory to be moved.
 *  - to: destiny path.
 * Returns: SUCCESS OR FAIL
 */
int tfsMove(char *from, char *to) {

  char c[MAX_INPUT_SIZE];
  int size;

  size = sprintf(c,"m %s %s",from, to);

  if (sendto(sockfd, c, size, 0, (struct sockaddr *) &serv_addr, servlen) < 0) {
    perror("client: sendto error");
    exit(EXIT_FAILURE);
  } 

  if (recvfrom(sockfd, buffer, sizeof(buffer), 0,(struct sockaddr *) &serv_addr, &servlen) < 0) {
    perror("client: recvfrom error");
    exit(EXIT_FAILURE);
  }

  if(atoi(buffer) < 0)
    return FAIL;
  else
    return SUCCESS;
  
  exit(EXIT_FAILURE);
}

/*
 * Sends lookup command to server and receives a message
 * Input:
 *  - path: path to the file or directory to be searched.
 * Returns: SUCCESS OR ERROR
 */
int tfsLookup(char *path) {

  char c[MAX_INPUT_SIZE];
  int size;

  size = sprintf(c,"l %s",path);

  if (sendto(sockfd, c, size, 0, (struct sockaddr *) &serv_addr, servlen) < 0) {
    perror("client: sendto error");
    exit(EXIT_FAILURE);
  } 

  if (recvfrom(sockfd, buffer, sizeof(buffer), 0,(struct sockaddr *) &serv_addr, &servlen) < 0) {
    perror("client: recvfrom error");
    exit(EXIT_FAILURE);
  } 

  if(atoi(buffer) < 0)
    return ERROR;
  else
    return SUCCESS;
  
  exit(EXIT_FAILURE);
}

/*
 * Sends print command to server and receives a message
 * Input:
 *  - file: output file name.
 * Returns: SUCCESS OR ERROR
 */
int tfsPrint(char *file){

  char c[MAX_INPUT_SIZE];
  int size;

  size = sprintf(c,"p %s",file);

  if (sendto(sockfd, c, size, 0, (struct sockaddr *) &serv_addr, servlen) < 0) {
    perror("client: sendto error");
    exit(EXIT_FAILURE);
  } 

  if (recvfrom(sockfd, buffer, sizeof(buffer), 0,(struct sockaddr *) &serv_addr, &servlen) < 0) {
    perror("client: recvfrom error");
    exit(EXIT_FAILURE);
  }

  if(atoi(buffer) < 0)
    return ERROR;
  else
    return SUCCESS;

  exit(EXIT_FAILURE);
}

/*
 * Creates socket and binds with it
 * Input:
 *  - sockPath: path to the server socket.
 * Returns: SUCCESS OR FAIL
 */
int tfsMount(char * sockPath) {

  if ((sockfd = socket(AF_UNIX, SOCK_DGRAM, 0) ) < 0) {
    perror("client: can't open socket");
    exit(EXIT_FAILURE);
  }

  int pid = getpid();
  pid = pid/10;

  sprintf(nameSocket,"/tmp/socket%d", pid);

  unlink(nameSocket);

  clilen = setSockAddrUn (nameSocket, &client_addr);
  if(bind(sockfd, (struct sockaddr *) &client_addr, clilen) < 0){
    perror("Client: bind error");
    exit(EXIT_FAILURE);
  }

  servlen = setSockAddrUn(sockPath, &serv_addr);

  return SUCCESS;
}

/*
 * Closes socket
 * Returns: SUCCESS
 */
int tfsUnmount() {

  close(sockfd);
  unlink(nameSocket);

  return SUCCESS;
}
