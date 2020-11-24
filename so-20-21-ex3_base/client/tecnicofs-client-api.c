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

int sockfd;
socklen_t servlen, clilen;
struct sockaddr_un serv_addr, client_addr;
char buffer[MAX_INPUT_SIZE];

int setSockAddrUn(char *path, struct sockaddr_un *addr) {

  if (addr == NULL)
    return 0;

  bzero((char *)addr, sizeof(struct sockaddr_un));
  addr->sun_family = AF_UNIX;
  strcpy(addr->sun_path, path);

  return SUN_LEN(addr);
}

int tfsCreate(char *filename, char nodeType) {

  char c[MAX_INPUT_SIZE];
  int size;

  size = sprintf(c,"c %s %c",filename, nodeType);

  if (sendto(sockfd, c, size, 0, (struct sockaddr *) &serv_addr, servlen) < 0) {
    perror("client: sendto error");
    exit(EXIT_FAILURE);
  } 

  printf("Vou receber\n");
  if (recvfrom(sockfd, buffer, sizeof(buffer), 0,(struct sockaddr *) &serv_addr, &servlen) < 0) {
    perror("client: recvfrom error");
    exit(EXIT_FAILURE);
  } 

  printf("recebi\n");
  
  if(atoi(buffer) < 0)
    exit(EXIT_FAILURE);
  else 
    exit(EXIT_SUCCESS);
  
  return 0;
}

int tfsDelete(char *path) {

  char c[MAX_INPUT_SIZE];
  int size;

  size = sprintf(c,"d %s",path);

  if (sendto(sockfd, c, size, 0, (struct sockaddr *) &serv_addr, servlen) < 0) {
    perror("client: sendto error");
    exit(EXIT_FAILURE);
  } 

  printf("Vou receber\n");
  if (recvfrom(sockfd, buffer, sizeof(buffer), 0,(struct sockaddr *) &serv_addr, &servlen) < 0) {
    perror("client: recvfrom error");
    exit(EXIT_FAILURE);
  } 

  printf("recebi\n");

  const char *a = "2";
  if(atoi(a) < 0)
    exit(EXIT_FAILURE);
  else 
    exit(EXIT_SUCCESS);


  return 0;
}

int tfsMove(char *from, char *to) {
  return -1;
}

int tfsLookup(char *path) {

  char c[MAX_INPUT_SIZE];
  int size;

  size = sprintf(c,"l %s",path);

  if (sendto(sockfd, c, size, 0, (struct sockaddr *) &serv_addr, servlen) < 0) {
    perror("client: sendto error");
    exit(EXIT_FAILURE);
  } 

  printf("Vou receber\n");
  if (recvfrom(sockfd, buffer, sizeof(buffer), 0,(struct sockaddr *) &serv_addr, &servlen) < 0) {
    perror("client: recvfrom error");
    exit(EXIT_FAILURE);
  } 

  printf("recebi\n");

  const char *a="2";
  if(atoi(a) < 0)
    exit(EXIT_FAILURE);
  else 
    exit(EXIT_SUCCESS);

  return -1;
}

int tfsMount(char * sockPath) {

  if ((sockfd = socket(AF_UNIX, SOCK_DGRAM, 0) ) < 0) {
    perror("client: can't open socket");
    exit(EXIT_FAILURE);
  }

  unlink("/tmp/socket");

  clilen = setSockAddrUn ("/tmp/socket", &client_addr);
  if(bind(sockfd, (struct sockaddr *) &client_addr, clilen) < 0){
    perror("Client: bind error");
    exit(EXIT_FAILURE);
  }

  servlen = setSockAddrUn(sockPath, &serv_addr);

  return 0;
}

int tfsUnmount() {
  return -1;
}
