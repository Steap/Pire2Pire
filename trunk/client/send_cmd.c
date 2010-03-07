#include <stdlib.h>
#include <stdio.h>
#include <netinet/in.h> // sockaddr_in, socket (), ...
#include <unistd.h>     // close ()
#include <string.h>     // strlen ()
#include <errno.h>      // errno

#define BUFFER_SIZE 9
#define TIMEOUT     1   // seconds

void
send_cmd (const char *cmd, struct sockaddr_in dest_addr) {
    int     dest_sock;
    int     nb_sent;
    int     nb_sent_sum;
    int     send_size;
    int     nb_received;
    char    buffer[BUFFER_SIZE];
    int     buffer_offset = 0;
    char    *response = NULL;
    fd_set  sock_set;
    int     select_return;

    struct timeval      timeout;

    dest_sock = socket (AF_INET, SOCK_STREAM, 0);
    if (dest_sock < 0) {
        perror ("socket");
        exit (EXIT_FAILURE);
    }
    if (connect (dest_sock,
                 (struct sockaddr *)&dest_addr,
                 sizeof (dest_addr)) < 0) {
        perror ("connect");
        exit (EXIT_FAILURE);
    }

    // strlen (cmd) + 1 to send '\n'
    send_size = strlen (cmd) + 1;

    // Send the command
    nb_sent_sum = 0;
    while (nb_sent_sum < send_size) {
        nb_sent = send (dest_sock,
                        cmd + nb_sent_sum,
                        send_size - nb_sent_sum,
                        0);
        if (nb_sent < 0) {
            perror ("send");
            exit (EXIT_FAILURE);
        }
        nb_sent_sum += nb_sent;
    }

    FD_ZERO (&sock_set);
    FD_SET (dest_sock, &sock_set);
    timeout.tv_sec = TIMEOUT;
    timeout.tv_usec = 0;    // seems recommended...

    // Get the response
    for (;;) {
        select_return = select (dest_sock + 1, &sock_set, NULL, NULL, &timeout);
        if (select_return < 0) {
            perror ("select");
            exit (EXIT_FAILURE);
        }
        if (select_return == 0) {
            // '\n' is within cmd supposedly...
            printf ("\n< Daemon timed out for request : %s", cmd);
            exit (EXIT_FAILURE);
        }        

        nb_received = recv (dest_sock, buffer, BUFFER_SIZE - 1, 0);
        if (nb_received < 0) {
            perror ("recv");
            exit (EXIT_FAILURE);
        }
        buffer[nb_received] = '\0';

        if ((response = realloc (response,
                                 buffer_offset + nb_received + 1)) == NULL) {
            perror ("realloc");
            exit (EXIT_FAILURE);
        }

        strcpy (response + buffer_offset, buffer);
        buffer_offset += nb_received;

        response[buffer_offset] = '\0';

        if (response[buffer_offset - 1] == '\n'
            || response[buffer_offset - 1] == '\0'
            || response[buffer_offset - 1] == '\r')
            break;
    }

    printf("< %s\n", response);
    free (response);
    close (dest_sock);
}
