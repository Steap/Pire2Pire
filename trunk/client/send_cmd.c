#include <sys/socket.h>     // send ()

#include <pthread.h>        // pthread_t
#include <signal.h>         // pthread_kill ()
#include <stdlib.h>         // exit ()
#include <stdio.h>          // perror ()
#include <string.h>         // strlen ()

#include "recv_resp.h"      // recv_resp ()
#include "util/prompt.h"    // prompt ()

pthread_t       prompt_thread;

void *
send_cmd (void *arg) {
    int         dest_sock = *((int *)arg);
    int         nb_sent;
    int         nb_sent_sum;
    int         send_size;
    char        *user_input;
    pthread_t   recv_thread;

    prompt_thread = pthread_self ();

    pthread_create (&recv_thread, NULL, recv_resp, &dest_sock);

    for (;;) {
        user_input = prompt ();

        send_size = strlen (user_input);

        // Send the command
        nb_sent_sum = 0;
        while (nb_sent_sum < send_size) {
            nb_sent = send (dest_sock,
                            user_input + nb_sent_sum,
                            send_size - nb_sent_sum,
                            0);
            if (nb_sent < 0) {
                perror ("send");
                exit (EXIT_FAILURE);
            }
            nb_sent_sum += nb_sent;
        }

        if (strncmp (user_input, "quit", 4) == 0) {
            free (user_input);
            break;
        }

        free (user_input);
    }

    // Kill the other thread
    if (pthread_kill (recv_thread, SIGINT) < 0) {
        perror ("pthread_kill");
        exit (EXIT_FAILURE);
    }
    // Wait for him to be properly killed (shouldn't last long)
    if (pthread_join (recv_thread, NULL) < 0) {
        perror ("pthread_join");
        exit (EXIT_FAILURE);
    }
    // Then die
    if (pthread_detach (pthread_self ()) < 0) {
        perror ("pthread_detach");
        exit (EXIT_FAILURE);
    }
    return NULL;
}
