#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>

#include "util.h"

void
socket_close (int fd)
{
    if (shutdown (fd, SHUT_RDWR) < 0)
        ERROR_HANDLER ("Shutdown");

    if (close (fd) < 0)
        ERROR_HANDLER ("Close");
}
