#include "tecnicofs-client-api.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>

#define MAX_INPUT_SIZE 100

int sockfd;
socklen_t servlen, clilen;
struct sockaddr_un serv_addr, client_addr;
void *buffer;
char *pathServer;

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
  int counter = 0, i;

  c[0] = 'c';
  c[1] = ' ';
  for(i = 2; i < strlen(filename); i++){
    printf("%ld",strlen(filename));
    c[i]=filename[counter];
    counter++;
  }
  c[counter+1] = ' ' ;
  c[counter+2] = nodeType;
  c[counter+3] = '\0';

  for(i=0; i<strlen(c); i++){
    printf("%c",c[i]);
  }

  servlen = setSockAddrUn(pathServer, &serv_addr);

  if (sendto(sockfd, c, strlen(c)+1, 0, (struct sockaddr *) &serv_addr, servlen) < 0) {
    perror("client: sendto error");
    exit(EXIT_FAILURE);
  } 
  if (recvfrom(sockfd, buffer, sizeof(buffer), 0, 0, 0) < 0) {
    perror("client: recvfrom error");
    exit(EXIT_FAILURE);
  }
  if((int*)buffer<0)
    exit(EXIT_FAILURE);
  else
    exit(EXIT_SUCCESS);
}

int tfsDelete(char *path) {
  return -1;
}

int tfsMove(char *from, char *to) {
  return -1;
}

int tfsLookup(char *path) {
  return -1;
}

int tfsMount(char * sockPath) {

  pathServer = malloc(sizeof(char)*MAX_INPUT_SIZE);
  pathServer = sockPath;

  if ((sockfd = socket(AF_UNIX, SOCK_DGRAM, 0) ) < 0) {
    perror("client: can't open socket");
    exit(EXIT_FAILURE);
  }

  unlink(sockPath);

  clilen = setSockAddrUn (sockPath, &client_addr);
  if(bind(sockfd, (struct sockaddr *) &client_addr, clilen) < 0){
    perror("Client: bind error");
    exit(EXIT_FAILURE);
  }
  return 0;
}

int tfsUnmount() {
  return -1;
}
