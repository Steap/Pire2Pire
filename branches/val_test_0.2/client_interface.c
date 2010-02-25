#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <arpa/inet.h>

#include "util.h"

#define BUFFSIZE 2
#define MAX_CONN 10
#define PORT 1338

FILE                *log_file;
pid_t               father_pid;
struct sockaddr_in  my_addr;

static void quit (int status);
static void handle_client (int sock, struct sockaddr_in addr);
static void logger (const char *msg);
static void handle_get (int sock, struct sockaddr_in addr, char *msg);

void
client_interface (pid_t pid) {
    // File descriptors for manager and new connection
    int                     sock, conn_sock;
    struct sockaddr_in      echo_client;
    socklen_t               size_echo_client;
    int                     yes = 1;

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
    my_addr.sin_addr.s_addr = INADDR_ANY;
    my_addr.sin_port        = htons (PORT);

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
handle_client (int sock, struct sockaddr_in addr){
    /* The message typed by the user might be longer than BUFFSIZE */
    char    *message;
    /* Number of chars written in message */ 
    int     ptr = 0;
    char    buffer[BUFFSIZE];
    int     n_received;
    char    port[6];    // 65535 + '\0'

    /* BUFFSIZE + 1, so the terminating null byte ('\0') can be stored */
    if ((message = calloc (BUFFSIZE + 1, sizeof (char))) == NULL) {
        perror ("Allocating memory for message");
        quit (EXIT_FAILURE);
    }

    log_file = fopen ("client_interface.log", "a");
    if (log_file == NULL) {
        perror ("fopen");
        exit (EXIT_FAILURE);
    }

    sprintf(port, "%d", ntohs (addr.sin_port));

    logger ("Handling a new client: ");
    logger (inet_ntoa (addr.sin_addr));
    logger (":");
    logger (port);
    logger ("\n");

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
            logger ("< [");
            logger (inet_ntoa (addr.sin_addr));
            logger (":");
            logger (port);
            logger ("] ");

            if (IS_CMD (message, "list")) {
                // TODO
                logger ("list\n");
            }
            else if (IS_CMD (message, "file")) {
                // TODO
                logger ("file\n");
            }
            else if (IS_CMD (message, "get")) {
                logger ("get\n");
                handle_get (sock, addr, message);
            }
            else if (IS_CMD (message, "traffic")) {
                // TODO
                logger ("traffic\n");
            }
            else if (IS_CMD (message, "ready")) {
                // TODO
                logger ("ready\n");
            }
            else if (IS_CMD (message, "checksum")) {
                // TODO
                logger ("checksum\n");
            }
            else if (IS_CMD (message, "neighbourhood")) {
                // TODO
                logger ("neighbourhood\n");
            }
            else if (IS_CMD (message, "neighbour")) {
                // TODO
                logger ("neighbour\n");
            }
            else if (IS_CMD (message, "redirect")) {
                // TODO
                logger ("redirect\n");
            }
            else if (IS_CMD (message, "error")) {
                // TODO
                logger ("error\n");
            }
            else if (IS_CMD (message, "exit")) {
                logger ("exit\n");
                break;
            }
            else {
                logger ("Received unknown command: ");
                logger (message);
                socket_write (sock, "error <");
                string_remove_trailer (message);
                socket_write (sock, message);
                socket_write (sock, "> Unknown command\n");
            }

            if ((message = realloc (message, BUFFSIZE+1)) == NULL) {
                fprintf (stderr, "Error reallocating memory for message");
                quit (EXIT_FAILURE);
            }
            ptr = 0;
        }
    }

    free (message);
    if (fclose (log_file) == EOF) {
        perror ("fclose");
    }

    exit (EXIT_SUCCESS);
}



static void
quit (int status) {
    printf("Exiting client interface properly\n");
    kill (father_pid, SIGUSR2);
    exit (status);
}



static void
logger (const char *msg) {
    if (fprintf(log_file, "%s", msg) < 0) {
        fprintf(stderr, "fprintf");
        quit (EXIT_FAILURE);
    }
    if (fflush (log_file) == EOF) {
        perror ("fflush");
        quit (EXIT_FAILURE);
    }
}



// TODO: encapsulate those multiple parameters?
static void
handle_get (int sock, struct sockaddr_in addr, char *msg) {
    char port[6];   // 65535 + '\0'
    // TODO: do we need to limit the size of a filename? to what length?
    char filename[257];

    // Avoiding \r and \n to screw sprintf
    string_remove_trailer (msg);

    // Scanning the request for filename
    if (sscanf (msg, "get %s", filename) == EOF) {
        perror ("sscanf");
        socket_write (sock, "error <");
        socket_write (sock, msg);
        socket_write (sock, "> Unable to parse filename\n");
        return;
    }

    if (access (filename, R_OK) == -1) {
        if (errno == ENOENT) {
            socket_write (sock, "error <");
            socket_write (sock, msg);
            socket_write (sock, "> File '");
            socket_write (sock, filename);
            socket_write (sock, "' does not exist\n");
        }
        else {
            perror ("access");
            socket_write (sock, "error <");
            socket_write (sock, msg);
            socket_write (sock, "> Unable to access file '");
            socket_write (sock, filename);
            socket_write (sock, " for reading'\n");
        }
    }

/*
    file = open (filename, O_RDONLY);
    if (file == -1) {
        if (errno == ENOENT) {
            socket_write (sock, "error <");
            socket_write (sock, msg);
            socket_write (sock, "> File '");
            socket_write (sock, filename);
            socket_write (sock, "' does not exist\n");
        }
        else {
            perror ("open");
            socket_write (sock, "error <");
            socket_write (sock, msg);
            socket_write (sock, "> Unable to open file '");
            socket_write (sock, filename);
            socket_write (sock, "'\n");
        }
        return;
    }
*/

    else {
        // TODO: enhancing the message
        socket_write (sock, "ready ");
        socket_write (sock, filename);
        socket_write (sock, " ");
        socket_write (sock, inet_ntoa (my_addr.sin_addr));
        socket_write (sock, ":");
        sprintf(port, "%d", ntohs (my_addr.sin_port));
        socket_write (sock, port);
        socket_write (sock, "\n");
    }
}
