#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>

#define PORT 2345
#define NB_QUEUE 10
#define BUF_SIZE 4

int conn = -1, sd = -1;

int main(void) {
  int yes = -1;
  struct sockaddr_in sa;

  sd = socket(AF_INET, SOCK_STREAM, 0);
  if(sd == -1) {
    perror("socket");
    return EXIT_FAILURE;
  }

  sa.sin_family = AF_INET;
  sa.sin_addr.s_addr = INADDR_ANY;
  sa.sin_port = htons(PORT);

  if(setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
    perror("setsockopt");
    exit(EXIT_FAILURE);
  }

  if(bind(sd, (struct sockaddr *)&sa, sizeof(sa)) == -1) {
    perror("bind");
    return EXIT_FAILURE;
  }

  if(listen(sd, NB_QUEUE)) {
    perror("listen");
    return EXIT_FAILURE;
  }

  conn = accept(sd, NULL, NULL);
  if(conn == -1) {
    perror("conn");
    return EXIT_FAILURE;
  }

  if(shutdown(conn, SHUT_RDWR) == -1) {
    perror("shutdown conn");
  }
  if(shutdown(sd, SHUT_RDWR) == -1) {
    perror("shutdown sd");
  }
  if(close(conn) == -1) {
    perror("close conn");
  }
  if(close(sd) == -1) {
    perror("close sd");
  }

  return EXIT_SUCCESS;
}
