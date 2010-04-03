#ifndef SOCKET_H
#define SOCKET_H

#include <netinet/in.h>     // struct sockaddr_in

/* Initializes a socket properly */
int socket_init (struct sockaddr_in *sa);

/* Reads a full line on a socket and returns it without '\n' */
char *socket_getline (int src_sock);

/* Reads a full line on a socket */
char *socket_getline_with_trailer (int src_sock);

/* Reads a full line without its trailer or times out */
char *socket_try_getline (int src_sock, int timeout);

/* Reads a full line or times out */
char *socket_try_getline_with_trailer (int src_sock, int timeout);

/* Sends msg on given socket */
void socket_sendline (int dest_sock, const char *msg);

#endif//SOCKET_H
