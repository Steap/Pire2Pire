#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>     // inet_ntoa
#include <netinet/in.h>     // sockaddr_in, inet_ntoa
#include <arpa/inet.h>      // inet_ntoa
#include <string.h>         // strcpy, strstr

#include "handle_get.h"
#include "../util/logger_util.h"
#include "../util/socket_util.h"
#include "../util/string_util.h"

#define BUFFSIZE    128

void
handle_client (int sock, struct sockaddr_in addr){
    /* The message typed by the user might be longer than BUFFSIZE */
    char    *message;
    /* Number of chars written in message */ 
    int     ptr = 0;
    char    buffer[BUFFSIZE];
    int     n_received;
    char    port[6];    // 65535 + '\0'
    FILE    *log_file;

    /* BUFFSIZE + 1, so the terminating null byte ('\0') can be stored */
    if ((message = calloc (BUFFSIZE + 1, sizeof (char))) == NULL) {
        perror ("Allocating memory for message");
        // TODO: warn someone
        exit (EXIT_FAILURE);
    }

    log_file = fopen ("client.log", "a");
    if (log_file == NULL) {
        perror ("fopen");
        // TODO: warn someone
        exit (EXIT_FAILURE);
    }

    sprintf(port, "%d", ntohs (addr.sin_port));
    // FIXME: Build the whole string before logging
    logger (log_file, "Handling a new client: ");
    logger (log_file, inet_ntoa (addr.sin_addr));
    logger (log_file, ":");
    logger (log_file, port);
    logger (log_file, "\n");

    while ((n_received = recv (sock, buffer, BUFFSIZE, 0)) != 0)
    {
        if (n_received < 0) {
            perror ("recv");
            // TODO: warn someone
            exit (EXIT_FAILURE);
        }
        /*
         * Seems necessary, unless we can guarantee that we will only receive
         * well-formed strings... 
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

        if (strstr (buffer, "\n") != NULL) {
            // FIXME: Build the whole string before logging
            logger (log_file, "< [");
            logger (log_file, inet_ntoa (addr.sin_addr));
            logger (log_file, ":");
            logger (log_file, port);
            logger (log_file, "] ");

            if (IS_CMD (message, "list")) {
                // TODO
                logger (log_file, "list\n");
            }
            else if (IS_CMD (message, "file")) {
                // TODO
                logger (log_file, "file\n");
            }
            else if (IS_CMD (message, "get")) {
                logger (log_file, "get\n");
                handle_get (sock, addr, message);
            }
            else if (IS_CMD (message, "traffic")) {
                // TODO
                logger (log_file, "traffic\n");
            }
            else if (IS_CMD (message, "ready")) {
                // TODO
                logger (log_file, "ready\n");
            }
            else if (IS_CMD (message, "checksum")) {
                // TODO
                logger (log_file, "checksum\n");
            }
            else if (IS_CMD (message, "neighbourhood")) {
                // TODO
                logger (log_file, "neighbourhood\n");
            }
            else if (IS_CMD (message, "neighbour")) {
                // TODO
                logger (log_file, "neighbour\n");
            }
            else if (IS_CMD (message, "redirect")) {
                // TODO
                logger (log_file, "redirect\n");
            }
            else if (IS_CMD (message, "error")) {
                // TODO
                logger (log_file, "error\n");
            }
            else if (IS_CMD (message, "exit")) {
                logger (log_file, "exit\n");
                break;
            }
            else {
                logger (log_file, "Received unknown command: ");
                logger (log_file, message);
                socket_write (sock, "error <");
                string_remove_trailer (message);
                socket_write (sock, message);
                socket_write (sock, "> Unknown command\n");
            }

            if ((message = realloc (message, BUFFSIZE+1)) == NULL) {
                fprintf (stderr, "Error reallocating memory for message");
                // TODO: warn someone
                exit (EXIT_FAILURE);
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
