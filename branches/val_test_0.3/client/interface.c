#include <stdlib.h>
#include <stdio.h>
#include <signal.h>     // signal
#include <unistd.h>     // fork, close

#include "handle_client.h"
#include "handle_get.h"

#define MAX_CONN    10
#define BUFFSIZE    128

pid_t               father_pid;
struct sockaddr_in  my_addr;

static void quit (int status);

void
client_interface (pid_t pid, int port) {
    // File descriptors for manager and new connection
    int                     sock, conn_sock;
    struct sockaddr_in      echo_client;
    socklen_t               size_echo_client;
    int                     yes = 1;
    FILE                    *log_file;

    printf ("Launching client interface\n");

    father_pid = pid;

    // FIXME: Shouldn't we create another log_file for this new session?
    // Truncates the log file
    log_file = fopen ("client_interface.log", "w");
    if (log_file == NULL) {
        perror ("fopen");
        exit (EXIT_FAILURE);
    }
    if (fclose (log_file) == EOF) {
        perror ("fclose");
        exit (EXIT_FAILURE);
    }

    if (signal (SIGINT, quit) == SIG_ERR) {
        perror ("signal");
        quit (EXIT_FAILURE);
    }

    my_addr.sin_family      = AF_INET;
    my_addr.sin_addr.s_addr = htonl (INADDR_ANY);
    my_addr.sin_port        = htons (port);

    if ((sock = socket (AF_INET, SOCK_STREAM, 0)) < 0) {
        perror ("socket");
        quit (EXIT_FAILURE);
    }

    /* 
     * FIXME: is that the truly beautiful way to avoid getting "Address already
     * used" errors from bind ? Even Cedric Augonnet does not know... We iz
     * doomed. 
     */
    if (setsockopt (sock, 
                    SOL_SOCKET, 
                    SO_REUSEADDR, 
                    &yes, 
                    sizeof (int)) < 0) {
        perror ("setsockopt");
        quit (EXIT_FAILURE);
    }

    if (bind (sock, 
              (struct sockaddr *) &my_addr, 
              sizeof (my_addr)) < 0) {
        perror ("bind");
        quit (EXIT_FAILURE);
    }

    if (listen (sock, MAX_CONN) < 0) {
        perror ("listen");
        quit (EXIT_FAILURE);
    }

    for(;;) {
        size_echo_client = sizeof (struct sockaddr_in);
        conn_sock = accept (sock,
                            (struct sockaddr *)&echo_client,
                            &size_echo_client);
        if(conn_sock < 0) {
            perror ("accept");
            quit (EXIT_FAILURE);
        }

        switch (fork ()) {
            case -1:
                perror ("fork");
                quit (EXIT_FAILURE);
                break;
            case 0:
                close (sock);
                handle_client (conn_sock, echo_client);
                break;
            default:
                close (conn_sock);
                // The interface continues to listen and accept
                break;
        }
    }

    quit (EXIT_SUCCESS);
}



static void
quit (int status) {
    printf("Exiting client interface properly\n");
    kill (father_pid, SIGUSR2);
    exit (status);
}
