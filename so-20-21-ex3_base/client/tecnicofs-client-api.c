#include "tecnicofs-client-api.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>

int setSockAddrUn(char *path, struct sockaddr_un *addr) {

  if (addr == NULL)
    return 0;

  bzero((char *)addr, sizeof(struct sockaddr_un));
  addr->sun_family = AF_UNIX;
  strcpy(addr->sun_path, path);

  return SUN_LEN(addr);
}

int tfsCreate(char *filename, char nodeType) {
  return -1;
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
  int sockfd;
  socklen_t servlen, clilen;
  struct sockaddr_un serv_addr, client_addr;

  clilen = setSockAddrUn (argv[1], &client_addr);
  if(bind(sockfd, (struct sockaddr *) &sockPath, clilen) < 0){
    perror("Client: bind error");
    exit(EXIT_FAILURE);
  }
  return -1;
}

int tfsUnmount() {
  return -1;
}
