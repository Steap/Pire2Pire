#include <stdlib.h>
#include <stdio.h>
#include <string.h>     // strlen
#include <unistd.h>     // close

#include <sys/socket.h>

void
socket_write (int socket, const char *msg) {
    if(send (socket, msg, strlen (msg), 0) < 0)
        perror ("write");
}

void
socket_close (int fd)
{
    if (shutdown (fd, SHUT_RDWR) < 0)
        perror ("shutdown");

    if (close (fd) < 0)
        perror ("close");
}

