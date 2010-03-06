#include <stdlib.h>
#include <stdio.h>
#include <netinet/in.h> // sockaddr_in, socket (), ...
#include <unistd.h>     // close ()
#include <string.h>     // strlen ()

void
send_cmd (const char *cmd, struct sockaddr_in dest_addr) {
    int     dest_sock;
    int     nb_sent;
    int     nb_sent_sum;
    int     send_size;
/* TODO: uncomment this and below if the daemon actually responds
    int     nb_received;
    char    *response;
    char    buffer[BUFFER_SIZE];
    int     buffer_offset = 0;
*/
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

    // strlen (cmd) + 1 to send the terminating null byte '\0'
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
/* TODO: uncomment this and above if the daemon actually responds
    // Get the response
    // BUFFER_SIZE + 1 to store the terminating null byte '\0'
    if ((response = calloc (BUFFER_SIZE + 1, sizeof (char))) == NULL) {
        perror ("calloc");
        exit (EXIT_FAILURE);
    }
    while ((nb_received = recv (dest_sock, buffer, BUFFER_SIZE, 0)) != 0) {
        if (nb_received < 0) {
            perror ("recv");
            exit (EXIT_FAILURE);
        }
        buffer[nb_received] = '\0';
        strcpy (response + buffer_offset, buffer);
        if ((response = realloc (response,
                                 buffer_offset + BUFFER_SIZE + 1)) == NULL) {
            perror ("realloc");
            exit (EXIT_FAILURE);
        }
        buffer_offset += nb_received;
        response[buffer_offset] = '\0';
    }

    printf("< %s\n", response);
    free (response);
*/
    close (dest_sock);
}
