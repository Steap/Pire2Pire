#ifndef CLIENT_HANDLE_CLIENT
#define CLIENT_HANDLE_CLIENT

#include <netinet/in.h>     // sockaddr_in

void handle_client (int sock, struct sockaddr_in addr);

#endif
