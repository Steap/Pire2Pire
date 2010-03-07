#ifndef CLIENT_HANDLER
#define CLIENT_HANDLER

#include <netinet/in.h>
#define MAX_THREADS  100

/*
 * Takes care of the client in a new thread.
 */
void handle_client (int client_socket, struct sockaddr_in *);
#endif /* CLIENT_HANDLER */
