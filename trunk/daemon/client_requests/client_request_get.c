#include <stdio.h>              // FILE
#include <string.h>             // strlen ()

#include "../../util/logger.h"  // log_failure ()
#include "../client.h"          // client_send ()
#include "../client_request.h"  // struct client_request
#include "../util/cmd.h"        // struct option

#define HASH_SIZE 32

extern FILE* log_file;

/* 
 * According to the protocol, there are no options available for the "get"
 * command.
 */
static struct option options[] = {
    {NULL, NULL, 0}    
};

#include <unistd.h>
void*
client_request_get (void *arg) {
    int                     argc;
    char                    **argv;
    int                     c;
    int                     nb_arguments;
    struct client_request   *r;

#define MAX_ANSWER_SIZE 256

    char   answer[MAX_ANSWER_SIZE];

    r = (struct client_request *) arg;    
    argv = cmd_to_argc_argv (r->cmd, &argc);
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
    if (nb_arguments == 0) {
        sprintf (answer,
                 " < get : you need to specify a key.\n");
    }
    else {
        if (strlen (argv[optind]) != HASH_SIZE) {
            sprintf (answer,
                     " < get : hash size is not good, man.\n");
        }
        else {
            sprintf (answer, 
                     " < get : should be getting file whose key is %s.\n",
                     argv[optind]);
        }
    }

    if (client_send (r->client, answer) < 0) {
        log_failure (log_file, "do_set () : failed to send data to the client");
    }
    
    cmd_free (argv);

    return NULL;
}
