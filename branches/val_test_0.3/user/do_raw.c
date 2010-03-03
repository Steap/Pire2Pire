#include <stdio.h>
#include <stdlib.h>

#include "../util/cmd.h"


static struct option options[] = {
    {NULL, NULL, 0}
};

void
do_raw (char *cmd) {
    int      argc;
    char     **argv;
    int      c;
    int      n_args;

    
    printf ("$ %s\n", cmd);

    argv = cmd_to_argc_argv (cmd, &argc);

    while ((c = cmd_get_next_option (argc, argv, options)) > 0);

    if (c == CMD_NOT_ALLOWED_OPTION ||
        c == CMD_MISSING_ARGUMENT) {
        cmd_print_error_message ();
        return;
    }

    n_args = argc - optind;
    cmd_free (argv);
    free (cmd);
    return;
}
