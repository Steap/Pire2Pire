#include <stdlib.h>
#include <sys/socket.h>
#include <errno.h>

#include "../../util/logger.h"

#define NB_QUEUE 10

extern FILE *log_file;

int
socket_init (struct sockaddr_in *sa) {
    int sd;
    int yes = 1;

    if ((sd = socket (AF_INET, SOCK_STREAM, 0)) < 0) {
        log_failure (log_file, "Client socket creation failed.");
        return -1;
    }
    else {
        log_success (log_file, "Created client socket.");
    }

    if (setsockopt (sd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof (int)) < 0) {
        log_failure (log_file, "Setsockopt failed.");
    }

    if (bind (sd, (struct sockaddr *)sa, sizeof (*sa)) < 0) {
        log_failure (log_file, "Could not assign a local address using bind : %d");
        return -1;
    }
     
    if (listen (sd, NB_QUEUE) < 0) {
        log_failure (log_file, "The socket could not be marked as a passive one.");
    }
    else {
        log_success (log_file, "Server is ready (port : %d).", ntohs (sa->sin_port));
    }

    return sd;
}
