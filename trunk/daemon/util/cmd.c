#include <stdio.h>  // sprintf ()
#include <stdlib.h> // NULL
#include <string.h> // strcmp

#include "cmd.h"    // struct option

#define ERROR_SIZE    256

int optind = 1;
char *optarg = NULL;
char opterror[ERROR_SIZE];

static int
is_a_delimiter (char c) {
    return (c == ' ' || c == '\0');
}
static void
error_not_allowed_option (char *o)
{
//    fprintf (stderr, "The option %s is not allowed\n", o);
    if (sprintf (opterror, "The option %s is not allowed\n", o) < 0)
        fprintf (stderr, "ERROR \n");
}

static void
error_requires_argument (char *o)
{
//    fprintf (stderr, "The option %s requires an argument\n", o);
    if (sprintf (opterror , "The option %s requires an argument\n", o) < 0)
        fprintf (stderr, "Error while sprintfing !\n");
}

static int
is_allowed_option (char *option, struct option *options)
{
    int i;
    struct option tmp;
    for (i = 0, tmp = options[i]; tmp.short_name != NULL; tmp = options[++i])
    {
        if (strcmp (option, tmp.short_name) == 0)
        {
            return 1;
        }
    }
    return 0;
}

static int
requires_argument (char *option, struct option *options)
{
    int i;
    struct option tmp;
    for (i = 0, tmp = options[i]; tmp.short_name != NULL; tmp = options[++i])
    {
        if (strcmp (option, tmp.short_name) == 0)
        {
            return tmp.has_arg;
        }
    }
    /* You should never get here */
    return no_argument;
}

void
cmd_print_error_message () {
    fprintf (stdout, "Error : %s\n", opterror);
}

int
cmd_get_next_option (int argc, char *argv[], struct option *options)
{
    if (optind == argc ||
        argv[optind][0] != '-')
    {
        return CMD_END_OF_OPTIONS;
    }
    if (argv[optind][0] == '-')
    {
        if (!is_allowed_option ((argv[optind]+1), options))
        {
            error_not_allowed_option (argv[optind]);
            return CMD_NOT_ALLOWED_OPTION;
//            exit (EXIT_FAILURE);
        }
        else
        {
            if (requires_argument (argv[optind]+1, options))
            {
                if (optind+1 == argc ||
                    argv[optind+1][0] == '-') // No argument
                {
                    error_requires_argument (argv[optind]);
                    return CMD_MISSING_ARGUMENT;
                 //   exit (EXIT_FAILURE);
                }
                optarg = argv[optind+1];
                optind += 2;
                return argv[optind-2][1];
            }
            else
            {
                optind++;
                return argv[optind-1][1]; 
            }
        }
    }
    return 0;
}

char**
cmd_to_argc_argv (const char *s, int *argc)
{
    int    i,j;
    int    beginning;
    char   *buffer;
    char   **argv;

    *argc = 0;
    if ((argv = malloc (sizeof (char *))) == NULL)
        return NULL;
    argv[0] = NULL;
    for (i = 0, beginning = 0; ; i++) {
        if (is_a_delimiter (s[i])) {
            if ((buffer = malloc (i-beginning+1)) == NULL) {
                exit (1);
            }
            for (j = 0; j < i - beginning ; j++) {
                buffer[j] = s[beginning+j];
            }
            buffer[j] = '\0';
   
            (*argc)++; 
            argv = realloc (argv, (1+*argc) * sizeof (char *));
            if (!argv)
                return NULL;

            argv[*argc-1] = strdup (buffer);
            argv[*argc]   = NULL;
            free (buffer);
            beginning = i+1;

            if (s[i] == '\0')
                break;
        }
    }
    return argv;
}

void
cmd_free (char **argv) {
    int i;
    for (i = 0; argv[i]; i ++)
        free (argv[i]);
    free (argv);
}
