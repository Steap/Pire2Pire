#ifndef CLIENT_HANDLE_GET
#define CLIENT_HANDLE_GET

#include <netinet/in.h>     // sockaddr_in

void handle_get (int sock, struct sockaddr_in addr, char *msg);

#endif
