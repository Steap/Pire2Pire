#include <stdlib.h>
#include <sys/socket.h>
#include <errno.h>
#include <netinet/in.h>
#include <string.h>

#include "../../util/logger.h"
#include "../../util/string.h"

#define NB_QUEUE            10
#define SOCKET_BUFFSIZE     128

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





char *
socket_getline (int src_sock) {
    char    *message = NULL;
    int     nb_received;
    char    buffer[SOCKET_BUFFSIZE];
    int     buffer_offset;

    buffer_offset = 0;

    for (;;) {
        nb_received = recv (src_sock, buffer, SOCKET_BUFFSIZE - 1, 0);
        if (nb_received < 0) {
            log_failure (log_file, "socket_getline () : recv failed");
            return NULL;
        }
        buffer[nb_received] = '\0';

        if ((message = realloc (message,
                            buffer_offset + nb_received + 1)) == NULL) {
            log_failure (log_file, "socket_getline () : realloc failed");
            return NULL;
        }

        strcpy (message + buffer_offset, buffer);
        buffer_offset += nb_received;

        message[buffer_offset] = '\0';

        if (message[buffer_offset - 1] == '\n')
            break;
    }

    string_remove_trailer (message);
    return message;
}

