#ifndef DAEMON_HANDLER
#define DAEMON_HANDLER

#include <netinet/in.h>

/*
 * Takes care of the daemon in a new thread.
 */
void handle_daemon (int daemon_socket, struct sockaddr_in *);

void *handle_requests (void *arg);

#endif//DAEMON_HANDLER
