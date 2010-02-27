#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>

#include "util.h"

// 128 seems a good compromise
#define BUFFSIZE    64
#define KEYSIZE     16

char    *cmd;
pid_t   father_pid;

// Pre-declarations
static void do_get ();
static void do_help ();
static void quit ();

void
user_interface (pid_t pid) {
    /* Number of chars written in cmd */ 
    int      ptr = 0;          
    char     buffer[BUFFSIZE];
    int      n_received;

    printf ("Launching user interface\n");

    father_pid = pid;

    if (signal (SIGINT, quit) == SIG_ERR) {
        perror ("signal");
        quit ();
    }
    
    /* BUFFSIZE + 1, so the terminating null byte ('\0') can be stored */
    if ((cmd = calloc (BUFFSIZE + 1, sizeof (char))) == NULL) {
        fprintf (stderr, "Allocating memory for user command");
        quit ();
    }

    /* Displays a prompt-like thing */
    printf("\n$> ");
    if (fflush(stdout) == EOF) {
        perror ("fflush");
        quit ();
    }

    while ((n_received = read (STDIN_FILENO, buffer, BUFFSIZE)) != 0) {
        if (n_received < 0) {
            perror ("read");
            quit ();
        }
        /*
         * Seems necessary, unless we can guarantee that we will only receive
         * well-formed strings... 
         */
        buffer[n_received] = '\0';

        strcpy (cmd + ptr, buffer); 
        cmd = realloc (cmd, ptr + 2*BUFFSIZE + 1);
        ptr += BUFFSIZE;
        cmd[ptr] = '\0';
 
        /* 
         * Upon receiving end of transmission, treating data
         */  
        if (strstr (buffer, "\n") != NULL)
        {
            if (IS_CMD(cmd, "set")) {
                printf ("> set\n");
            }
            else if (IS_CMD (cmd, "help")) {
                do_help ();
            }
            else if (IS_CMD (cmd, "list")) {
                printf ("> list\n");
            }
            else if (IS_CMD (cmd, "get")) {
                do_get (cmd);
            }
            else if (IS_CMD (cmd, "info")) {
                printf ("> info\n");
            }
            else if (IS_CMD (cmd, "download")) {
                printf ("> download\n");
            }
            else if (IS_CMD (cmd, "upload")) {
                printf ("> upload\n");
            }
            else if (IS_CMD (cmd, "connect")) {
                printf ("> connect\n");
            }
            else if (IS_CMD (cmd, "raw")) {
                printf ("> raw\n");
            }
            else if (IS_CMD (cmd, "quit")) {
                printf ("Goodbye\n");
                break; // not quit () unless you free cmd
            }
            else {
                printf("Unknown command. Type help for the command list.\n");
            }

            if ((cmd = realloc (cmd, BUFFSIZE+1)) == NULL) {
                fprintf (stderr, "Allocating memory for user command");
                quit ();
            }
            ptr = 0;

            printf ("\n$> ");
            if (fflush (stdout) == EOF) {
                perror ("fflush");
                quit ();
            }
        }
    }

    quit ();
}



static void
do_get () {
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
        quit (EXIT_FAILURE);
    }
    do {
        n_received = recv (sock, buffer, BUFFSIZE, 0);

        if (n_received < 0) {
            perror ("recv");
            quit (EXIT_FAILURE);
        }
        if (n_received != 0) {
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



static void
do_help () {
    printf("\
-= HELP =-\n\
set                         lists the options available\n\
set [OPTION=VALUE] [...]    changes the value of an option\n\
help                        displays this help\n\
list                        lists the resources available on the network\n\
get KEY                     gets the resource identified by KEY\n\
info                        displays statistics\n\
download                    information about current downloads\n\
upload                      information about current uploads\n\
connect IP:PORT             connects to another program\n\
raw IP:PORT CMD             sends a command to another program\n\
quit                        exits the program\n\
");
}



static void
quit () {
    printf ("Exiting user interface properly\n");
    free (cmd);
    if (kill (father_pid, SIGUSR1) < 0) {
        perror ("kill");
        exit (EXIT_FAILURE);
    }
    exit (EXIT_SUCCESS);
}
