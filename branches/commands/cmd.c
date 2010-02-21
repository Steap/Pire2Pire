/*
 * Command handling.
 *
 * TODO : Error cases.
 * TODO : handle different option formats ? "foo=bar" is currently supported,
 * but we may want to support something like "-f" or "--foo".
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cmd.h"

static void
option_free (struct option *o) {
    if (o != NULL) {
        if (o->name != NULL)
            free (o->name);
        if (o->value != NULL)
            free (o->value);
        free (o);
    }
    
}

char*
option_get_name (const struct option *o) {
    if (o != NULL)
        return o->name;
    return NULL;
}

char*
option_get_value (const struct option *o) {
    if (o != NULL) 
        return o->value;
    return NULL;
}

/*
 * It seems like using strtok is not a good idea (cf. man 3 strtok).
 * That's why we're doing all the crap by ourselves.
 */

#define IN_NAME         1
#define IN_OPTIONS      2
#define IN_ARGS         3
struct command*
cmd_create (const char *s)
{
    struct command          *cmd;
    int                     i, j;
    int                     beginning;
    int                     where;
    int                     n_options, n_args;
    char                    *buffer;

    where     = IN_NAME;
    n_options = 1;
    n_args    = 1;

    if (s == NULL) {
        exit (1);
    }

    if ((cmd = malloc (sizeof (struct command))) == NULL) {
        exit (1);
    }

    if ((cmd->options = malloc (sizeof (struct option*))) == NULL) {
        exit (1);
    }
    cmd->options[0] = NULL; 

    if ((cmd->args = (char **) malloc (sizeof (char *))) == NULL) {
        exit (1);
    }
    cmd->args[0] = NULL;

    for (i = 0, beginning = 0; ; i++) {
        if (s[i] == ' ' || s[i] == '\0') {
            if ((buffer = malloc (i-beginning+1)) == NULL) {
                exit (1);
            }
            for (j = 0; j < i - beginning ; j++) {
                buffer[j] = s[beginning+j];
            }
            buffer[j] = '\0';

            /* Options are over */
            if (where == IN_OPTIONS && strstr (buffer, "=") == NULL)
                where = IN_ARGS;
           
            /* Options after arguments : invalid*/
            if (where == IN_ARGS && strstr (buffer, "=") != NULL)
                 exit (1);

            switch (where) {
            case IN_NAME:
                cmd->name = malloc (i-beginning+1);
                strcpy (cmd->name, buffer);
                where = IN_OPTIONS;
                break;
            case IN_OPTIONS:
                if ((cmd->options = 
                        realloc (cmd->options, 
                                 ++n_options*sizeof(struct option*))) == NULL){
                    exit (1);
                }
                cmd->options[n_options-1] = NULL;
                cmd->options[n_options - 2] = malloc (sizeof (struct option));
                if (cmd->options[n_options - 2] == NULL) {
                    exit (1);
                }
                
                cmd->options[n_options - 2]->value 
                    = strdup (strstr (buffer, "=") +1);
                /* Hum, this is hackety hack */
                buffer[strstr(buffer, "=")-buffer] = '\0';
                cmd->options[n_options - 2]->name 
                    = strdup (buffer);
                break;
            case IN_ARGS:
                if ((cmd->args = realloc (cmd->args,
                                          ++n_args*sizeof (char *))) == NULL) {
                    exit (1);
                }
                cmd->args[n_args - 1] = NULL;
                cmd->args[n_args - 2] = malloc (i - beginning +1);
                strcpy (cmd->args[n_args - 2], buffer);
                break;
            default:    
                break;
            }
            free (buffer);
            beginning = i+1;

            if (s[i] == '\0')
                break;
        }
    }
    return cmd;
}

char*
cmd_get_name (const struct command *cmd) {
    return cmd->name;
}

char*
cmd_get_next_arg (const struct command *cmd) {
    static int i = 0;
    return cmd->args[i++];
}

struct option*
cmd_get_next_option (const struct command *cmd) {
    static int i = 0;
    return cmd->options[i++];
}

void
cmd_free (struct command *c)
{
    int i;
    if (c == NULL)
        return;
    if (c->name != NULL)
        free (c->name);
    if (c->options != NULL) {
        for (i = 0; c->options[i] != NULL; i++) {
            option_free (c->options[i]);
        }
        free (c->options);   
    } 
    if (c->args != NULL) {
        for (i = 0; c->args[i] != NULL; i++) {
            free (c->args[i]);
        }
        free (c->args);
    }
    free (c);
}
