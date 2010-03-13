#include <stdlib.h>
#include <stdio.h>
#include <netinet/in.h> // sockaddr_in, socket (), ...
#include <unistd.h>     // close ()
#include <string.h>     // strlen ()
#include <errno.h>      // errno
#include <signal.h>     // signal ()
#include <pthread.h>    // pthread_detach (), ...

#define BUFFER_SIZE 128

char *response = NULL;

void
proper_quit (int signum) {
    (void)signum;
    // We ensure the free-ing of resources
    if (response)
        free (response);
    // Then we leave properly
    pthread_detach (pthread_self ());
    pthread_exit (NULL);
}

void *
recv_resp (void *arg) {
    int     src_sock = *((int *)arg);
    int     nb_received;
    char    buffer[BUFFER_SIZE];
    int     buffer_offset;
    struct sigaction    quit_action;

    sigemptyset(&quit_action.sa_mask);
    quit_action.sa_handler = proper_quit;
    quit_action.sa_flags = 0;
    // We expect a SIGINT from parent thread
    if (sigaction (SIGINT, &quit_action, NULL) < 0) {
        perror ("sigaction");
        proper_quit (SIGINT);
    }

    // Infinite loop of recv and printf
    for (;;) {
        buffer_offset = 0;

        // This loop recvs until a terminating byte is read
        for (;;) {
            nb_received = recv (src_sock, buffer, BUFFER_SIZE - 1, 0);
            if (nb_received < 0) {
                perror ("recv");
                exit (EXIT_FAILURE);
            }
            buffer[nb_received] = '\0';

            if ((response = realloc (response,
                                buffer_offset + nb_received + 1)) == NULL) {
                perror ("realloc");
                exit (EXIT_FAILURE);
            }

            strcpy (response + buffer_offset, buffer);
            buffer_offset += nb_received;

            response[buffer_offset] = '\0';

            if (response[buffer_offset - 1] == '\n'
                || response[buffer_offset - 1] == '\0'
                || response[buffer_offset - 1] == '\r')
                break;
        }

        // Then we print the response recvd and loop
        printf("%s", response);
        //free (response);
        //response = NULL;
    }

    fprintf (stderr,
        "WARNING: Reached a section of code supposedly unreachable.\n");

    return NULL;
}
