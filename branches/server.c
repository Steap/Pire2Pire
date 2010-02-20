#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>

#include "util.h"

#define BUFFSIZE      2
#define MAX_CONN      2
#define PORT       1338

static int server_socket = 0;

/*
 * Used to kill the server properly when receiving SIGINT (^C).
 * TODO : this function should also kill all living sons of the main process,
 * and close the associated sockets.
 * (Should we store their pids in a global array ?)
 */
static void 
kill_server (int signum)
{
    /* Dummy assignment to shut the compiler up */
    signum = signum;
    socket_close (server_socket);
    exit (EXIT_SUCCESS);
}

/*
 * Function used by sons of the main process in order to deal with clients :
 * receiving data, sending back data... and, finally, closing all opened sockets
 * and exiting properly.
 */
static void
handle_client (int sock)
{
    /* The message sent by the client might be longer than BUFFSIZE */
    char     *message;
    /* Number of chars written in message */ 
    int      ptr = 0;          
    char     buffer[BUFFSIZE];
    int      n_received;

    
    /* BUFFSIZE + 1, so the terminating null byte ('\0') can be stored */
    if ((message = calloc (BUFFSIZE + 1, sizeof (char))) == NULL)
        ERROR_HANDLER ("Allocating memory for message");
    
    while ((n_received = recv (sock, buffer, BUFFSIZE, 0)) != 0)
    {
        if (n_received < 0)
            ERROR_HANDLER ("recv");
        /*
         * Seems necessary, unless we can guarantee that we will only receive
         * well-formed strings... 
         */
        buffer[n_received] = '\0';

        /* Sending back the data */
        if (send (sock, buffer, n_received, 0) != n_received)
            ERROR_HANDLER ("send");


        strcpy (message+ptr, buffer);       
        message = realloc (message, ptr+2*BUFFSIZE+1);
        ptr+=BUFFSIZE;
        message[ptr] = '\0';
        
        /* 
         * Upon receiving end of transmission, treating data
         */  
        if (strstr (buffer, "\n") != NULL)
        {
            fprintf (stdout, "Server has just received : %s\n", message);
            if ((message = realloc (message, BUFFSIZE+1)) == NULL)
                ERROR_HANDLER ("Allocating memory for message");
            ptr = 0;
        }
    }
    free (message);

    socket_close (sock);

    exit (EXIT_SUCCESS);
}

int
main (void)
{
    int                   client_socket;
    struct sockaddr_in    server_echo, client_echo;
    int                   yes = 1;

    if ((server_socket = socket (AF_INET, SOCK_STREAM, 0)) < 0)
        ERROR_HANDLER ("socket");

    /* In case the server is killed using ^C */
    signal (SIGINT, kill_server);

    server_echo.sin_family      = AF_INET; 
    server_echo.sin_addr.s_addr = INADDR_ANY;        
    server_echo.sin_port        = htons (PORT);

    /* 
     * FIXME : is that the truly beautiful way to avoid getting "Address already
     * used" errors from bind ? Even Cedric Augonnet does not know... We iz
     * doomed. 
     */
    if (setsockopt (server_socket, 
                    SOL_SOCKET, 
                    SO_REUSEADDR, 
                    &yes, 
                    sizeof (int)) < 0)
        ERROR_HANDLER ("setsockopt");

    if (bind (server_socket, 
              (struct sockaddr *) &server_echo, 
              sizeof (server_echo)) < 0)
        ERROR_HANDLER ("bind");

    if (listen (server_socket, MAX_CONN) < 0)
        ERROR_HANDLER ("listen");

    for (;;)
    {
        unsigned int len = sizeof (client_echo);
        /* Is that really useful to fill in the client_echo variable ?*/
        if ((client_socket = accept (server_socket, 
                                     (struct sockaddr *) &client_echo,
                                     &len)) < 0)
            ERROR_HANDLER ("accept");
        fprintf (stderr, "Connection accepted\n");
        switch (fork ())
        {
            case -1:
                ERROR_HANDLER ("fork");
                break;
            case 0:
                handle_client (client_socket);
                break;
            default:
                break;
        }
    }

    /* FIXME : is there anyway that we actually get here ? */
    socket_close (server_socket);

    return EXIT_SUCCESS;
}
