#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>

#include "util.h"

// 128 seems a good compromise
#define BUFFSIZE 128

char    *cmd;
pid_t   father_pid;

// Pre-declarations
static void help ();
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
                printf("> set\n");
            }
            else if (IS_CMD (cmd, "help")) {
                help ();
            }
            else if (IS_CMD (cmd, "list")) {
                printf("> list\n");
            }
            else if (IS_CMD (cmd, "get")) {
                printf("> get\n");
            }
            else if (IS_CMD (cmd, "info")) {
                printf("> info\n");
            }
            else if (IS_CMD (cmd, "download")) {
                printf("> download\n");
            }
            else if (IS_CMD (cmd, "upload")) {
                printf("> upload\n");
            }
            else if (IS_CMD (cmd, "connect")) {
                printf("> connect\n");
            }
            else if (IS_CMD (cmd, "raw")) {
                printf("> raw\n");
            }
            else if (IS_CMD (cmd, "quit")) {
                printf("Goodbye\n");
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
quit                        exits the program\n\
");
}



static void
quit () {
    printf ("Exiting user interface properly\n");
    free (cmd);
    if (kill (father_pid, SIGUSR1) == -1) {
        perror ("kill");
        exit (EXIT_FAILURE);
    }
    exit (EXIT_SUCCESS);
}
