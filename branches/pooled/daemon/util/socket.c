#include <netinet/in.h>         // struct sockaddr_in

#include <errno.h>              // errno
#include <stdio.h>              // FILE
#include <string.h>             // strcpy ()

#include "../../util/logger.h"  // log_failure ()
#include "../../util/string.h"  // string_remove_trailer ()
#include "socket.h"             // socket_getline_with_trailer ()

#define NB_QUEUE            10

extern FILE *log_file;

int
socket_init (struct sockaddr_in *sa) {
    int sd;
    int yes = 1;

    if ((sd = socket (AF_INET, SOCK_STREAM, 0)) < 0) {
        log_failure (log_file, "socket_init (): Socket creation failed.");
        return -1;
    }
    else {
        log_success (log_file, "socket_init (): Created socket.");
    }

    if (setsockopt (sd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof (int)) < 0) {
        log_failure (log_file, "socket_init (): setsockopt () failed.");
    }
#include <errno.h>
#if 0
bind:
    if (bind (sd, (struct sockaddr *)sa, sizeof (*sa)) < 0) {
        if (errno == EADDRINUSE) {
        log_failure (log_file,
                    "socket_init: bind () failed, error: %s %d",
                    strerror (errno),
                    ntohs (sa->sin_port));
        sa->sin_port = htons (1 + ntohs (sa->sin_port));
        log_failure (log_file,
                    "socket_init: bind () : trying again with %d...\n",
                     ntohs (sa->sin_port));
        goto bind;
                    
        }
        return -1;
    }
#endif
    errno = 0;
    do {
        if (errno == EADDRINUSE) {
            log_failure (log_file,
                         "%s : bind () failed, error %s (%d). Trying with %d",
                         __func__,
                         strerror (errno),
                         ntohs (sa->sin_port),
                         ntohs (sa->sin_port)+1);
            sa->sin_port = htons (1 + ntohs (sa->sin_port));
        }
        else if (errno != 0) { /* Unhandled errors */
            log_failure (log_file, 
                         "%s : bind () failed, error %s",
                         __func__,
                         strerror (errno));
        }
    } while (bind (sd, (struct sockaddr *) sa, sizeof (*sa)) < 0);

    if (listen (sd, NB_QUEUE) < 0) {
        log_failure (log_file,
        "socket_init (): The socket could not be marked as a passive one.");
    }
    else {
        log_success (log_file,
                    "socket_init (): Socket ready (port : %d).",
                    ntohs (sa->sin_port));
    }

    return sd;
}

char *
socket_getline (int src_sock) {
    return string_remove_trailer (socket_getline_with_trailer (src_sock));
}

char *
socket_getline_with_trailer (int src_sock) {
    char    *message = NULL;
    int     nb_received;
    char    character;
    int     buffer_offset;

    buffer_offset = 0;

    for (;;) {
        /*
         * BUGFIX: Here we do not want to received more than one character at
         * a time, so that we can stop readling a line on '\n'
         */
        nb_received = recv (src_sock, &character, 1, 0);
        if (nb_received < 0) {
            log_failure (log_file, "socket_getline () : recv failed (%d : %s)",
                        src_sock,
                        strerror (errno));
            return NULL;
        }
        else if (nb_received == 0) {
            log_failure (log_file, "socket_getline () : empty line");
            return message;
        }

        // TODO: would be better to double the size each time needed
        if ((message = realloc (message, buffer_offset + 2)) == NULL) {
            log_failure (log_file, "socket_getline () : realloc failed");
            return NULL;
        }
        message[buffer_offset] = character;
        buffer_offset += 1;
        message[buffer_offset] = '\0';

        if (character == '\n')
            break;
    }

    return message;
}

char *
socket_try_getline (int src_sock, int timeout) {
    return string_remove_trailer (socket_try_getline_with_trailer (src_sock,
                                                                    timeout));
}

char *
socket_try_getline_with_trailer (int src_sock, int timeout_sec) {
    fd_set          socket_read_set;
    struct timeval  timeout;
    int             select_value;

    timeout.tv_sec = timeout_sec;
    timeout.tv_usec = 0;
    FD_ZERO (&socket_read_set);
    FD_SET (src_sock, &socket_read_set);
    select_value = select (src_sock + 1,
                            &socket_read_set,
                            NULL,
                            NULL,
                            &timeout);
    if (select_value < 0) {
        log_failure (log_file, "socket_try_getline: select () failed");
        return NULL;
    }
    else if (!select_value) /* Timeout */
        return NULL;
    else
        return socket_getline_with_trailer (src_sock);
}

void
socket_sendline (int dest_sock, const char *msg) {
    int         nb_sent;
    int         nb_sent_sum;
    int         send_size;

    send_size = strlen (msg);

    nb_sent_sum = 0;
    while (nb_sent_sum < send_size) {
        nb_sent = send (dest_sock,
                        msg + nb_sent_sum,
                        send_size - nb_sent_sum,
                        0);
        if (nb_sent < 0) {
            log_failure (log_file, "socket_sendline (): Failed to send");
            return;
        }
        nb_sent_sum += nb_sent;
    }

    log_send (log_file, msg);
}
