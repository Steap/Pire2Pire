#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>

#include "util.h"

void
socket_write (int socket, const char *msg) {
    if(write (socket, msg, strlen (msg)) == -1)
        perror ("write");
}

void
socket_close (int fd)
{
    if (shutdown (fd, SHUT_RDWR) < 0)
        ERROR_HANDLER ("Shutdown");

    if (close (fd) < 0)
        ERROR_HANDLER ("Close");
}

void
string_remove_trailer (char *msg) {
    char *to_be_replaced;

    to_be_replaced = strchr(msg, '\r');
    if (to_be_replaced != NULL) {
        *to_be_replaced = '\0';
    }
    to_be_replaced = strchr(msg, '\n');
    if (to_be_replaced != NULL) {
        *to_be_replaced = '\0';
    }
}
