#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>

#define PORT 2340
#define NB_QUEUE 10
#define BUF_SIZE 4

int conn = -1, sd = -1;

void kill_handler(int pid) {
  (void)pid;
  if(shutdown(sd, SHUT_RDWR) == -1) {
    perror("shutdown 2");
  }
  if(close(sd) == -1) {
    perror("close 2");
  }
  exit(EXIT_SUCCESS);
}

int main(void) {
  int nbread = -1, nbwritten = -1, retwrite = -1, yes = -1;
  bool quit = false;
  struct sockaddr_in sa;
  char buf[BUF_SIZE];
  pid_t pid = -1;

  signal(SIGINT, kill_handler);

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

 while(!quit) {
    conn = accept(sd, NULL, NULL);
    if(conn == -1) {
      perror("conn");
      return EXIT_FAILURE;
    }

    pid = fork();

    switch(pid) {
    case -1:
      perror("fork");
      return EXIT_FAILURE;
    case 0:
      do {
        nbread = read(conn, buf, sizeof(buf));
        if(nbread == -1) {
          perror("read");
          return EXIT_FAILURE;
        }
        nbwritten = 0;
        while(nbwritten < nbread) {
          retwrite = write(conn, buf + nbwritten, nbread - nbwritten);
          if(retwrite == -1) {
            perror("write");
            return EXIT_FAILURE;
          }
          nbwritten += retwrite;
        }
      } while(nbread > 0);
    default:
      break;
    }
  }

  return EXIT_SUCCESS;
}
