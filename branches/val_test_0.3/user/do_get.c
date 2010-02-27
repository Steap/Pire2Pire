#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>     // socket, ... 
#include <netinet/in.h>     // sockaddr_in, ...
#include <arpa/inet.h>      // inet_addr
#include <sys/stat.h>       // open
#include <fcntl.h>          // open
#include <unistd.h>         // write, close

#include "../util/string_util.h"
#include "../util/socket_util.h"

#define BUFFSIZE    128
#define KEYSIZE     16

void
do_get (char *cmd) {
    /* The message typed by the user might be longer than BUFFSIZE */
    char                *message;
    /* Number of chars written in message */ 
    int                 ptr = 0;
    char                buffer[BUFFSIZE];
    int                 n_received;
    char                filekey[KEYSIZE + 1]; // + 1 for '\0'
    long                beginning;
    long                end;
    int                 sock;
    struct sockaddr_in  addr;

    string_remove_trailer (cmd);
    if (sscanf (cmd, "get %s %ld %ld", filekey, &beginning, &end) == EOF) {
        perror ("sscanf");
        printf ("Error scanning your request. See stderr for details.\n");
        return;
    }

    // FIXME: the calling process should fork!
    switch (fork ()) {
        case -1:
            perror ("fork");
            exit (EXIT_FAILURE);
            break;
        case 0:
            break;
        default:
            return;
            break;
    }
    // Code written below is only executed by the child process

    free (cmd);

    // FIXME: for now, filekey is filename, must use a key instead
    // FIXME: for now, beginning and end are unused, must be implemented
    // FIXME: for now, server is statically located at 127.0.0.1:1338
    addr.sin_family = AF_INET;
    addr.sin_port = htons (1338);
    addr.sin_addr.s_addr = inet_addr ("127.0.0.1");

    sock = socket (AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror ("socket");
        printf ("Unable to open a new socket. Request cancelled.\n");
        return;
    }

    if (connect (sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror ("connect");
        printf ("Unable to connect to 127.0.0.1:1338. Request cancelled.\n");
        return;
    }

    socket_write (sock, "get ");
    socket_write (sock, filekey);
    socket_write (sock, "\n\0");

    if ((message = calloc (BUFFSIZE + 1, sizeof (char))) == NULL) {
        perror ("Allocating memory for message");
        // TODO: warn someone
        exit (EXIT_FAILURE);
    }
    do {
        n_received = recv (sock, buffer, BUFFSIZE, 0);

        if (n_received < 0) {
            perror ("recv");
            // TODO: warn someone
            exit (EXIT_FAILURE);
        }
        if (n_received != 0) {
            /*
             * Seems necessary, unless we can guarantee that we will only
             * receive well-formed strings... 
             */
            buffer[n_received] = '\0';

            strcpy (message + ptr, buffer);
            if((message = realloc (message, ptr + 2*BUFFSIZE + 1)) == NULL) {
                fprintf (stderr, "Error reallocating memory for message\n");
                // TODO: warn someone
                exit (EXIT_FAILURE);
            }
            ptr += n_received;
            message[ptr] = '\0';
        }
    } while (strstr (buffer, "\n") == NULL);

    if (IS_CMD (message, "ready")) {
        // TODO: outsource this fragment of code
        int                 port;
        int                 data_sock;
        struct sockaddr_in  data_addr;
        int                 file;
        char                data[BUFFSIZE];
        int                 nb_recv;
        int                 nb_written;
        int                 nb_written_sum;

        // Retrieve the data connection port
        string_remove_trailer(message);
        if (sscanf (message, "ready %*s %d", &port) == EOF) {
            perror ("sscanf");
            printf ("Unable to read IP and port from peer response.\n");
            return;
        }

        data_addr.sin_family = AF_INET;
        data_addr.sin_port = htons (port);
        data_addr.sin_addr.s_addr = addr.sin_addr.s_addr;

        data_sock = socket (AF_INET, SOCK_STREAM, 0);
        if (sock < 0) {
            perror ("socket");
            printf ("Unable to open a new socket for data.\n");
            return;
        }

        if (connect (data_sock,
                     (struct sockaddr *)&data_addr,
                     sizeof(data_addr)) < 0) {
            perror ("connect");
            printf ("Unable to connect to %s:%d.\n",
                    inet_ntoa (data_addr.sin_addr),
                    port);
            return;
        }

        // FIXME: Now where are we downloading this file to?
        file = open ("download", O_WRONLY|O_CREAT|O_TRUNC, (mode_t)0644);
        if (file < 0) {
            perror ("open");
            // FIXME: what file?
            printf ("Unable to open file ...\n");
            return;
        }

        do {
            nb_recv = recv (data_sock, data, BUFFSIZE, 0);
            if (nb_recv < 0) {
                perror ("recv");
                return;
            }

            if (nb_recv > 0) {
                nb_written_sum = 0;
                do {
                    nb_written = write (file, data, nb_recv - nb_written_sum);
                    if (nb_written < 0) {
                        perror ("write");
                        return;
                    }
                    nb_written_sum += nb_written;
                } while (nb_written_sum < nb_recv);
            }
        } while (nb_recv > 0);

        // TODO: name this file!
        printf ("Successfully received file into local file 'download'.\n");

        close (data_sock);
        close (file);
    }
    else if (IS_CMD (message, "error")) {
        printf ("%s", message);
        // TODO
    }
    else {
        printf ("Received unknown response from client: %s", message);
    }

    free (message);

    exit (EXIT_SUCCESS);
}
