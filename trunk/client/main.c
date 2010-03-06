#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h> // inet_aton ()
#include <netinet/in.h> // inet_aton ()
#include <arpa/inet.h>  // inet_aton ()
#include <unistd.h>     // fork (), close ()
#include <string.h>     // strncmp (), strlen ()

#include "send_cmd.h"
#include "util/prompt.h"

#define BUFFER_SIZE 2

#define HELP_TEXT "\
-= HELP =-\n\
set                         lists the options available\n\
set [OPTION=VALUE] [...]    changes the value of an option\n\
help                        displays this help\n\
list                        lists the resources available on the network\n\
get KEY                     gets the resource identified by KEY\n\
info                        displays statistics\n\
download                    information about current downloads\n\
upload                      information about current uploads\n\
connect IP:PORT             connects to another program\n\
raw IP:PORT CMD             sends a command to another program\n\
quit                        exits the program\n"

/* Lazy comparison of cmd and msg (stops at the end of cmd) */
#define IS_CMD(msg, cmd) (strncmp ((msg), (cmd), strlen (cmd)) == 0)

int main (int argc, char **argv) {
    int                 daemon_port;
    struct sockaddr_in  daemon_addr;
    char *              user_input;

    // Configure daemon IP and port
    switch (argc) {
        case 1:     // ./client
            // FIXME: 1338 must be replaces by a port read in daemon conf
            printf ("No [IP PORT] specified, using default 127.0.0.1:1338.\n");
            inet_aton ("127.0.0.1", &daemon_addr.sin_addr);
            daemon_addr.sin_port = htons (1338);
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
        if (IS_CMD (user_input, "help")) {
            printf(HELP_TEXT);
        }
        else if (IS_CMD (user_input, "quit")) {
            free (user_input);
            break;
        }
        else {
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
        }
        free (user_input);
        user_input = NULL;
    }

    printf("\n");

    return EXIT_SUCCESS;
}

// Send cmd to the address and print the response

