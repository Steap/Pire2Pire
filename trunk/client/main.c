#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h> // inet_aton ()
#include <netinet/in.h> // inet_aton ()
#include <arpa/inet.h>  // inet_aton ()
#include <readline/readline.h>
#include <readline/history.h>

static char * rl_gets ();

int main (int argc, char **argv) {
    int                 daemon_port;
    struct sockaddr_in  daemon_addr;

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
            server_addr.sin_port = htons (daemon_port);
            break;
        default:
            printf ("Usage : %s [IP PORT]\n", argv[0]);
            exit (EXIT_FAILURE);
    }

    // TODO: connect to the daemon
    // TODO: handle user input
    for (;;)
        if (rl_gets () == NULL)
            break;

    printf("\n");

    return EXIT_SUCCESS;
}

/* A static variable for holding the line. */
static char *line_read = (char *)NULL;

/* Read a string, and return a pointer to it.
   Returns NULL on EOF. */
static char *
rl_gets () {
    if (line_read) {
        free (line_read);
        line_read = (char *)NULL;
    }

    line_read = readline ("$> ");

    if (line_read && *line_read)
        add_history (line_read);

    return (line_read);
}
