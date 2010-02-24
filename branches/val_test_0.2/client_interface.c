#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>

#include "util.h"

#define BUFFSIZE 2
#define MAX_CONN 10
#define PORT 1338

FILE *log_file;
pid_t father_pid;

static void quit (int status);
static void handle_client (int sock);
static void logger (const char * msg);

void
client_interface (pid_t pid) {
    // File descriptors for manager and new connection
    int                     sock, conn_sock;
    struct sockaddr_in      echo;
    int                     yes = 1;

    printf ("Launching client interface\n");

    father_pid = pid;

    /*
     * FIXME: "a" rather than "w" ?
     * For test purposes, "w" is quite mkay
     */
    log_file = fopen ("client_interface.log", "w");
    if (log_file == NULL) {
        perror ("fopen");
        exit (EXIT_FAILURE);
    }

    if (signal (SIGINT, quit) == SIG_ERR) {
        perror ("signal");
        quit (EXIT_FAILURE);
    }

    echo.sin_family      = AF_INET;
    echo.sin_addr.s_addr = INADDR_ANY;
    echo.sin_port        = htons (PORT);

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
              (struct sockaddr *) &echo, 
              sizeof (echo)) < 0) {
        perror ("bind");
        quit (EXIT_FAILURE);
    }

    if (listen (sock, MAX_CONN) < 0) {
        perror ("listen");
        quit (EXIT_FAILURE);
    }

    for(;;) {
        /*
         * FIXME: add the others parameters
         */
        conn_sock = accept(sock, NULL, NULL);
        if(conn_sock == -1) {
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
                handle_client (conn_sock);
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
handle_client (int sock) {
    /* The message typed by the user might be longer than BUFFSIZE */
    char    *message;
    /* Number of chars written in message */ 
    int     ptr = 0;
    char    buffer[BUFFSIZE];
    int     n_received;

    /* BUFFSIZE + 1, so the terminating null byte ('\0') can be stored */
    if ((message = calloc (BUFFSIZE + 1, sizeof (char))) == NULL) {
        perror ("Allocating memory for message");
        quit (EXIT_FAILURE);
    }

    logger ("Handling a client\n");

    while ((n_received = recv (sock, buffer, BUFFSIZE, 0)) != 0)
    {
        if (n_received < 0) {
            perror ("recv");
            quit (EXIT_FAILURE);
        }
        /*
         * Seems necessary, unless we can guarantee that we will only receive
         * well-formed strings... 
         */
        buffer[n_received] = '\0';

        strcpy (message + ptr, buffer);
        if((message = realloc (message, ptr + 2*BUFFSIZE + 1)) == NULL) {
            fprintf (stderr, "Error reallocating memory for message\n");
            quit (EXIT_FAILURE);
        }
        ptr += BUFFSIZE;
        message[ptr] = '\0';

        if (strstr (buffer, "\n") != NULL) {
            if (IS_CMD (message, "list")) {
                logger ("< list\n");
            }
            else if (IS_CMD (message, "file")) {
                logger ("< file\n");
            }
            else if (IS_CMD (message, "get")) {
                logger ("< get\n");
            }
            else if (IS_CMD (message, "traffic")) {
                logger ("< traffic\n");
            }
            else if (IS_CMD (message, "ready")) {
                logger ("< ready\n");
            }
            else if (IS_CMD (message, "checksum")) {
                logger ("< checksum\n");
            }
            else if (IS_CMD (message, "neighbourhood")) {
                logger ("< neighbourhood\n");
            }
            else if (IS_CMD (message, "neighbour")) {
                logger ("< neighbour\n");
            }
            else if (IS_CMD (message, "redirect")) {
                logger ("< redirect\n");
            }
            else if (IS_CMD (message, "error")) {
                logger ("< error\n");
            }
            else if (IS_CMD (message, "exit")) {
                logger ("< exit\n");
                break;
            }
            else {
                logger ("Received unknown command : ");
                logger (message);
            }

            if ((message = realloc (message, BUFFSIZE+1)) == NULL) {
                fprintf (stderr, "Error reallocating memory for message");
                quit (EXIT_FAILURE);
            }
            ptr = 0;
        }
    }

    logger ("Client handled successfully\n");

    exit (EXIT_SUCCESS);
}



static void
quit (int status) {
    printf("Exiting client interface properly\n");
    if (fclose (log_file) == EOF) {
        perror ("fclose");
    }
    kill (father_pid, SIGUSR2);
    exit (status);
}



static void
logger (const char * msg) {
    if (fprintf(log_file, "%s", msg) < 0) {
        fprintf(stderr, "fprintf");
        quit (EXIT_FAILURE);
    }
    if (fflush (log_file) == EOF) {
        perror ("fflush");
        quit (EXIT_FAILURE);
    }
}
