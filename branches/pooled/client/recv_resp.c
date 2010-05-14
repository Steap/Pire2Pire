#include <sys/socket.h> // recv ()

#include <pthread.h>    // pthread_detach (), ...
#include <signal.h>     // sigaction (), signal ()
#include <stdlib.h>     // NULL
#include <stdio.h>      // perror ()
#include <string.h>     // strcpy ()

#define BUFFER_SIZE         128

char *response = NULL;

extern pthread_t    prompt_thread;

static void
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
    int                 src_sock;
    int                 nb_received;
    char                buffer[BUFFER_SIZE];
    int                 buffer_offset;
    struct sigaction    quit_action;

    src_sock = *((int *)arg);

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
            else if (nb_received == 0) {
                printf ("Server disconnected\nExiting\n");
                // TODO: Send a signal to parent thread and free memory
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

        printf ("%s", response);

        /*
         * We just wrote on the tty, but the user might have begun typing
         * his next command, so the display will screw its screen... In order
         * for him not to be screwed, we inform the prompt thread to display
         * him his input again
         */
        pthread_kill (prompt_thread, SIGUSR1);
    }

    fprintf (stderr,
        "WARNING: Reached a section of code supposedly unreachable.\n");

    return NULL;
}
