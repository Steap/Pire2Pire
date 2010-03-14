#include <arpa/inet.h>  // inet_aton ()
#include <netinet/in.h> // sockaddr_in

#include <pthread.h>    // pthread_create (), pthread_join ()
#include <stdlib.h>     // exit ()
#include <stdio.h>      // printf ()
#include <unistd.h>     // fork (), close ()

#include "send_cmd.h"   // send_cmd

int main (int argc, char **argv) {
    int                 daemon_port;
    struct sockaddr_in  daemon_addr;
    int                 daemon_sock;
    pthread_t           send_thread;

    // Configure daemon IP and port
    switch (argc) {
        case 1:     // ./client
            // FIXME: 1337 must be replaced by a port read in daemon conf
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

    daemon_sock = socket (AF_INET, SOCK_STREAM, 0);
    if (daemon_sock < 0) {
        perror ("socket");
        exit (EXIT_FAILURE);
    }
    if (connect (daemon_sock,
                 (struct sockaddr *)&daemon_addr,
                 sizeof (daemon_addr)) < 0) {
        perror ("connect");
        exit (EXIT_FAILURE);
    }

    pthread_create (&send_thread, NULL, send_cmd, &daemon_sock);
    pthread_join (send_thread, NULL);

    close (daemon_sock);

    return EXIT_SUCCESS;
}
