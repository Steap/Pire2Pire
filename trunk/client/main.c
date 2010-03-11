#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h> // inet_aton ()
#include <netinet/in.h> // inet_aton ()
#include <arpa/inet.h>  // inet_aton ()
#include <unistd.h>     // fork (), close ()
#include <string.h>     // strncmp (), strlen ()
#include <sys/types.h>  // waitpid ()
#include <sys/wait.h>   // waitpid ()

#include "send_cmd.h"
#include "util/prompt.h"

#define BUFFER_SIZE 2

int main (int argc, char **argv) {
    int                 daemon_port;
    struct sockaddr_in  daemon_addr;
    char *              user_input;

    // Configure daemon IP and port
    switch (argc) {
        case 1:     // ./client
            // FIXME: 1234 must be replaced by a port read in daemon conf
            printf ("No [IP PORT] specified, using default 127.0.0.1:1337.\n");
            inet_aton ("127.0.0.1", &daemon_addr.sin_addr);
            daemon_addr.sin_port = htons (1337);
            break;
        case 3:     // ./client IP PORT
            // Retrieve daemon IP
            if (inet_aton (argv[1], &daemon_addr.sin_addr) == 0) {
                printf ("Invalid IP address.\n");
                exit (EXIT_FAILURE);
            }
            // Retrieve then convert daemon port
            daemon_port = atoi (argv[2]);
            if (daemon_port < 1024 || daemon_port > 49151) {
                printf ("Please use a port between 1024 and 49151.\n");
                exit (EXIT_FAILURE);
            }
            daemon_addr.sin_port = htons (daemon_port);
            break;
        default:
            printf ("Usage : %s [IP PORT]\n", argv[0]);
            exit (EXIT_FAILURE);
    }
    daemon_addr.sin_family = AF_INET;

    for (;;) {
        user_input = prompt ();
        switch (fork ()) {
            case -1:
                perror ("fork");
                exit (EXIT_FAILURE);
            case 0:
                send_cmd (user_input, daemon_addr);
                free (user_input);
                exit (EXIT_SUCCESS);
            default:
                break;
        }
        // Only the father executes this, so user_input isn't freed
        if (strncmp(user_input, "quit", 4) == 0) {
            free (user_input);
            break;
        }
        else {
            free (user_input);
        }
    }

    // Should we wait for child processes to return?

    printf("\nGoodbye.\n");

    return EXIT_SUCCESS;
}
