#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>

#include "ui.h"
#include "util.h"

#define BUFFSIZE 2

/*
 * Checks whether msg begins or is cmd
 * It uses lazy evaluation to avoid accessing msg[] too far
 * TODO : maybe we could change the way it is checked...
 */
#define IS_CMD(msg,cmd) (                                   \
(strlen(msg) > strlen(cmd))                                 \
&& (msg[strlen(cmd)] == ' ' || msg[strlen(cmd)] == '\n')    \
&& (strncmp ((msg), (cmd), strlen(cmd)) == 0)               \
)

static void help ();

void
handle_input (pid_t server_pid) {
    /* The message typed by the user might be longer than BUFFSIZE */
    char     *message;
    /* Number of chars written in message */ 
    int      ptr = 0;
    char     buffer[BUFFSIZE];
    int      n_received;

    /* BUFFSIZE + 1, so the terminating null byte ('\0') can be stored */
    if ((message = calloc (BUFFSIZE + 1, sizeof (char))) == NULL)
        ERROR_HANDLER ("Allocating memory for message");

    /* Displays a prompt-like thing */
    printf("\n$> ");
    fflush(stdout);

    while ((n_received = read (STDIN_FILENO, buffer, BUFFSIZE)) != 0)
    {
        if (n_received < 0)
            ERROR_HANDLER ("recv");
        /*
         * Seems necessary, unless we can guarantee that we will only receive
         * well-formed strings... 
         */
        buffer[n_received] = '\0';

        strcpy (message+ptr, buffer);
        message = realloc (message, ptr+2*BUFFSIZE+1);
        ptr+=BUFFSIZE;
        message[ptr] = '\0';

        if (strstr (buffer, "\n") != NULL)
        {
            if (IS_CMD(message, "set")) {
                printf("> set\n");
            }
            else if (IS_CMD (message, "help")) {
                help ();
            }
            else if (IS_CMD (message, "list")) {
                printf("> list\n");
            }
            else if (IS_CMD (message, "get")) {
                printf("> get\n");
            }
            else if (IS_CMD (message, "info")) {
                printf("> info\n");
            }
            else if (IS_CMD (message, "download")) {
                printf("> download\n");
            }
            else if (IS_CMD (message, "upload")) {
                printf("> upload\n");
            }
            else if (IS_CMD (message, "connect")) {
                printf("> connect\n");
            }
            else if (IS_CMD (message, "raw")) {
                printf("> raw\n");
            }
            else if (IS_CMD (message, "quit")) {
                printf("Goodbye\n");
                break;
            }
            else {
                printf("Unknown command\n");
            }

            if ((message = realloc (message, BUFFSIZE+1)) == NULL)
                ERROR_HANDLER ("Allocating memory for message");
            ptr = 0;

        /* Prompt-like thing again */
        printf("\n$> ");
        fflush(stdout);

        }
    }

    kill (server_pid, SIGINT);

    exit (EXIT_SUCCESS);
}

static void
help () {
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
");
}

