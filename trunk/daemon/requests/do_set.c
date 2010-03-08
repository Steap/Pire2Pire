#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "../util/cmd.h"
#include "../callback_argument.h"

#include "../../util/logger.h"

extern FILE* log_file;

/* 
 * According to the protocol, there are no options available for the "set"
 * command.
 */
static struct option options[] = {
    {NULL, NULL, 0}    
};
#include <unistd.h>
void*
do_set (void *arg) {
    sleep (5);
    int         argc;
    char        **argv;
    int         c;
    int         nb_arguments;
    struct callback_argument *cba;

#define MAX_ANSWER_SIZE 256

    char   answer[MAX_ANSWER_SIZE];
    cba = (struct callback_argument *) arg;
    argv = cmd_to_argc_argv (cba->cmd, &argc);
    while ((c = cmd_get_next_option (argc, argv, options)) > 0);
    if (c == CMD_NOT_ALLOWED_OPTION) {
        /*
         * Check the protocol : errors (such as passing non-allowed arguments)
         * might have to be silenced. Which would be pretty damn stupid, but eh,
         * we ain't supposed to question that protocol.
         * TODO : write an RFC to change it.
         */
        cmd_print_error_message ();
        return NULL;
    }
   
    nb_arguments = argc - optind;
    switch (nb_arguments) {
        case 0:
            sprintf (answer, 
                     " < No arguments, should display all available options\n");
            break;
        case 1:
            sprintf (answer, " < Should have set %s\n", argv[optind]);
            break;
        /* More than one arg */
        default:
            sprintf (answer, " < You shall only set one option at a time...\n");
            break;
    } 

    if (send (cba->client_socket, answer, strlen (answer), 0) < 0) {
        /* We might be willing to switch errno here, blah blah */
        log_failure (log_file, "do_set() : failed to send data to the client");
    }
    
    cmd_free (argv);
    if (cba)
        cba_free (cba);

    return NULL;

}
