#include <stdio.h>
#include <stdlib.h>

#include "cmd.h"

int 
main (void)
{
    char message[] = "set foo=bar arg1 arg2";
    struct command *cmd;
    struct option *o;
    char *foo;

    cmd =  cmd_create (message);

    printf ("Command name : %s\n",
            cmd_get_name (cmd));
    printf ("==== Options ====\n");
    while ((o = cmd_get_next_option (cmd)) != NULL) {
        printf ("Opt : %s = %s\n",
                option_get_name  (o),
                option_get_value (o));
    }
              

    printf ("==== Args ====\n");
    while ((foo = cmd_get_next_arg (cmd)) != NULL)
        printf ("Arg : %s\n", foo);
    cmd_free (cmd);

    

    return 0;
}
