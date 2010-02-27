#include <stdlib.h>
#include <stdio.h>
#include <netinet/in.h> // sockaddr_in, ...
#include <unistd.h>     // access ()
#include <errno.h>      // errno
#include <sys/stat.h>   // open
#include <fcntl.h>      // open

#include "../util/socket_util.h"
#include "../util/string_util.h"

#define BUFFSIZE 128

// TODO: do we need a wrapper for all these parameters?
void
handle_get (int sock, struct sockaddr_in addr, char *msg) {
    char                port[6];    // strlen("65535") + 1 for '\0'
    // TODO: do we need to limit the size of a filename? to what length?
    char                filename[257];
    int                 data_sock;
    int                 data_conn_sock;
    struct sockaddr_in  data_addr;
    socklen_t           size_data_addr;
    struct sockaddr_in  data_conn_addr;
    socklen_t           size_data_conn_addr;
    int                 data_fd;
    char                data[BUFFSIZE];
    int                 nb_read;
    int                 nb_sent;
    int                 nb_sent_sum;

    // Avoiding \r and \n to screw sprintf
    string_remove_trailer (msg);

    // Scanning the request for filename
    if (sscanf (msg, "get %s", filename) == EOF) {
        perror ("sscanf");
        socket_write (sock, "error <");
        socket_write (sock, msg);
        socket_write (sock, "> Unable to parse filename.\n");
        return;
    }

    if (access (filename, R_OK) < 0) {
        if (errno == ENOENT) {
            socket_write (sock, "error <");
            socket_write (sock, msg);
            socket_write (sock, "> File '");
            socket_write (sock, filename);
            socket_write (sock, "' does not exist.\n");
        }
        else {
            perror ("access");
            socket_write (sock, "error <");
            socket_write (sock, msg);
            socket_write (sock, "> Unable to access file '");
            socket_write (sock, filename);
            socket_write (sock, "' for reading.\n");
        }
        return;
    }

    data_addr.sin_family = AF_INET;
    // We listen only to our client
    data_addr.sin_addr.s_addr = htonl (INADDR_ANY);
    // We let the OS decide a free port
    data_addr.sin_port = 0;

    if ((data_sock = socket (AF_INET, SOCK_STREAM, 0)) < 0) {
        perror ("socket");
        // TODO: warn the peer
        return;
    }

    if (bind (data_sock, 
              (struct sockaddr *) &data_addr, 
              sizeof (data_addr)) < 0) {
        perror ("bind");
        // TODO: warn the peer
        return;
    }

    // Retrieving the port chosen by the kernel
    size_data_addr = sizeof (data_addr);
    if (getsockname (data_sock,
                     (struct sockaddr *) &data_addr,
                     &size_data_addr) < 0) {
        perror ("getsockname");
        return;
    }

    switch (fork ()) {
        case -1:
            perror ("fork");
            exit (EXIT_FAILURE);
            break;
        case 0:
            close (sock);
            break;
        default:
            close (data_sock);
            // TODO: enhancing the message
            socket_write (sock, "ready ");
            socket_write (sock, filename);
            socket_write (sock, " ");
            sprintf(port, "%d", ntohs (data_addr.sin_port));
            socket_write (sock, port);
            socket_write (sock, "\n");
            return;
            break;
    }
    // The code written below is only executed by the child

    if (listen (data_sock, 1) < 0) {
        perror ("listen");
        // TODO: warn the peer
        return;
    }

    size_data_conn_addr = sizeof (struct sockaddr_in);
    data_conn_sock = accept (data_sock,
                             (struct sockaddr *)&data_conn_addr,
                             &size_data_conn_addr);
    if (data_conn_sock < 0) {
        perror ("accept");
        // TODO: warn the peer
        return;
    }

    // We don't need the accepting sock anymore
    close (data_sock);

    data_fd = open (filename, O_RDONLY);
    if (data_fd < 0) {
        perror ("open");
        // TODO: warn the peer
        return;
    }

    do {
        nb_read = read (data_fd, &data, BUFFSIZE);
        if (nb_read < 0) {
            perror ("read");
            // TODO: warn the peer
            return;
        }

        if (nb_read > 0) {
            nb_sent_sum = 0;
            do {
                nb_sent = send (data_conn_sock,
                                data,
                                nb_read - nb_sent_sum,
                                0); 
                if (nb_sent < 0) {
                    perror ("send");
                    // TODO: warn the peer
                    return;
                }
                nb_sent_sum += nb_sent;
            } while (nb_sent_sum < nb_read);
        }
    } while (nb_read > 0);

    close (data_conn_sock);
    close (data_fd);

    exit (EXIT_SUCCESS);
}
