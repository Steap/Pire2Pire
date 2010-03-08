#ifndef OPTION_H
#define OPTION_H

/*
 * Command line parser.
 *
 * Inspired by GNU getopt.
 */

/*
 * optind is the number of the argument being treated.
 */
extern int optind;

/*
 * When an option requires an argument, it is stored in optarg.
 */
extern char *optarg;

struct option 
{
    char *short_name;
    char *long_name; 
    int  has_arg; /* FIXME: Should be ENUM */
};

#define no_argument       0
#define required_argument 1

#define CMD_END_OF_OPTIONS        -1
#define CMD_NOT_ALLOWED_OPTION    -2
#define CMD_MISSING_ARGUMENT      -3

void cmd_print_error_message ();
/*
 * cmd_get_next_option parses the command line.
 *
 * Returns :
 *     -1 when the command line has been entirely parsed.
 *     The value of the current option otherwise.
 */
int cmd_get_next_option (int argc, char *argv[], struct option *options);

/*
 * Turns a command given as a char* into a table of pointers.
 *
 * The returned char** is similar to the well-known argv. *argc is updated so
 * that it is equals to the number of arguments in the command line.
 *
 * The returned value should be tested against NULL, which is returned if an
 * error occurs.
 */
char **cmd_to_argc_argv (const char *cmd, int *argc);

void cmd_free (char **argv);
#endif /* OPTION_H */
