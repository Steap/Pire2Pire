#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>  // pid_t
#include <unistd.h>     // read
#include <signal.h>     // signal, kill
#include <string.h>     // strcpy, strstr

#include "../util/string_util.h"

/* Please keep these in alphabetical order */
#include "do_connect.h"
#include "do_download.h"
#include "do_get.h"
#include "do_help.h"
#include "do_info.h"
#include "do_list.h"
#include "do_raw.h"
#include "do_set.h"
#include "do_upload.h"

#define BUFFSIZE    128

char    *cmd                 = NULL;
void    (*callback) (char *) = NULL;
pid_t   father_pid;

// Pre-declarations
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
                callback = &do_set;
            }
            else if (IS_CMD (cmd, "help")) {
                callback = &do_help;
            }
            else if (IS_CMD (cmd, "list")) {
                callback = &do_list;
            }
            else if (IS_CMD (cmd, "get")) {
                callback = &do_get;
            }
            else if (IS_CMD (cmd, "info")) {
                callback = &do_info;
            }
            else if (IS_CMD (cmd, "download")) {
                callback = &do_download;
            }
            else if (IS_CMD (cmd, "upload")) {
                callback = &do_upload;
            }
            else if (IS_CMD (cmd, "connect")) {
                callback = &do_connect;
            }
            else if (IS_CMD (cmd, "raw")) {
                callback = &do_raw;
            }
            else if (IS_CMD (cmd, "quit")) {
                printf ("Goodbye\n");
                break; // not quit () unless you free cmd
            }
            else {
                printf("Unknown command. Type help for the command list.\n");
                callback = NULL;
            }

            string_remove_trailer (cmd); 

            if (callback) {
                switch (fork ()) {
                case -1:
                    //
                    break;
                case 0:
                    (*callback) (cmd);
                    exit (EXIT_SUCCESS);
                    break;
                default:
                    break;
                }
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
quit () {
    printf ("Exiting user interface properly\n");
    
    if (cmd)
        free (cmd);

    
    if (kill (father_pid, SIGUSR1) < 0) {
        perror ("kill");
        exit (EXIT_FAILURE);
    }
    exit (EXIT_SUCCESS);
}
