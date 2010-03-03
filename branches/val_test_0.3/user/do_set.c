#include <stdio.h>
#include <stdlib.h>

#include "../util/cmd.h"

/* 
 * According to the protocol, there are no options available for the "set"
 * command.
 */
static struct option options[] = {
    {NULL, NULL, 0}    
};

void
do_set (char *cmd) {
    int         argc;
    char        **argv;
    int         c;
    int         nb_arguments;

    argv = cmd_to_argc_argv (cmd, &argc);

    while ((c = cmd_get_next_option (argc, argv, options)) > 0);
    if (c == CMD_NOT_ALLOWED_OPTION) {
        cmd_print_error_message ();
        return;
    }
   
    nb_arguments = argc - optind;
    switch (nb_arguments) {
        case 0:
            printf("No arguments, should display all available options\n");
            break;
        case 1:
            printf ("You asked for %s\n", argv[optind]);
            break;
        /* More than one arg */
        default:
            printf ("You shall only set one option at a time...\n");
            break;
    } 
    
    free (cmd);
    cmd_free (argv);
}
