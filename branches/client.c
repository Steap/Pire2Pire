#include <arpa/inet.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>

#include "util.h"

#define BUFFSIZE 32

static void
usage ()
{
    fprintf (stderr, "Usage : ./client <@IP> port\n");
}

int
main (int argc, char *argv[])
{
    int                   socket_fd;
    struct sockaddr_in    sa;
    int                   received = 0;
    char                  buffer[BUFFSIZE];

    if (argc != 3)
    {
        usage ();   
        exit (EXIT_FAILURE);
    }
 
    if ((socket_fd = socket (PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
        ERROR_HANDLER ("socket")

    /* FIXME : it seems that these values should not be edited directly.
     * man 7 ip plz */
    sa.sin_family      = AF_INET;
    sa.sin_addr.s_addr = inet_addr (argv[1]);
    sa.sin_port        = htons (atoi (argv[2])); 

    if (connect (socket_fd, (struct sockaddr *) &sa, sizeof (sa)) < 0)
        ERROR_HANDLER ("connect");
   
    fprintf (stdout, "Connected\n"); 

    int n_read = 0;
    /* n_read = 0 : the user stopped their client using ^D */
    while ((n_read = read (STDIN_FILENO, buffer, BUFFSIZE)) != 0)
    {
        if (n_read < 0)
            ERROR_HANDLER ("read");

        if (send (socket_fd, buffer, n_read, 0) != n_read)
            ERROR_HANDLER ("send");

        received = 0;
        while (received < n_read)
        {
            int bytes;
            if ((bytes = recv (socket_fd, buffer, BUFFSIZE-1, 0)) < 1)
                ERROR_HANDLER ("recv");
            received += bytes;
            buffer[bytes] = '\0';
            fprintf (stdout, "%s\n", buffer);
        }
    }

    if (close (socket_fd) < 0)
        ERROR_HANDLER ("close");

    fprintf (stdout, "Socket closed\n");
    fprintf (stdout, "End\n");

    return EXIT_SUCCESS;
}
