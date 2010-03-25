#include <stdio.h>
#include <stdlib.h>

#include "cmd_parser.h"

#define CMD_TO_PARSE "list -e foo caca -r pipi -g - lol --gay -- foo --exclude bar\n"
#define CMD_ERR1 "unknownoption -e lol -d prout noob"
#define CMD_ERR2 "missingargument -e"

static struct option_template options[] = {
    {'e', "exclude", ARG_REQUIRED},
    {'g', "gay", ARG_REQUIRED},
    {'r', "recursive", NO_ARG},
    {'t', "tomato", ARG_REQUIRED},
    {0, NULL, 0}
};

int main () {
    struct parsed_cmd   *pcmd;
    struct arg_list     *arg;
    int                 opt_index;
    char *              opt_value;
    char                opt_read;

    pcmd = cmd_parse (CMD_TO_PARSE, options);
    if (!pcmd)
        exit (EXIT_FAILURE);
    printf ("Command:\n%s\nOptions:\n", pcmd->cmd);
    opt_index = 0;
    while ((opt_read = cmd_get_next_opt (pcmd, options, &opt_index))) {
        switch (opt_read) {
            case 'e':
                printf ("exclude    ");
                break;
            case 'g':
                printf ("gay        ");
                break;
            case 'r':
                printf ("recursive");
                break;
            case 't':
                printf ("tomato     ");
                break;
            default:
            printf ("Error: unknown option set...");
        }
        if ((opt_value = cmd_get_opt_value (pcmd, options, &opt_index)))
            printf (" %s", opt_value);
        printf ("\n");
    }
    printf ("Arguments (%d):\n", pcmd->nb_arguments);
    arg = pcmd->arguments;
    while (arg) {
        printf ("%s\n", arg->text);
        arg = arg->next;
    }

    cmd_parse_free (pcmd);

    pcmd = cmd_parse (CMD_ERR1, options);
    if (pcmd == PARSER_MISSING_ARGUMENT)
        printf ("Missing argument in: %s\n", CMD_ERR1);
    else if (pcmd == PARSER_UNKNOWN_OPTION)
        printf ("Unknown option in: %s\n", CMD_ERR1);
    else
        printf ("Should have been an error...\n");
    cmd_parse_free (pcmd);

    pcmd = cmd_parse (CMD_ERR2, options);
    if (pcmd == PARSER_MISSING_ARGUMENT)
        printf ("Missing argument in: %s\n", CMD_ERR2);
    else if (pcmd == PARSER_UNKNOWN_OPTION)
        printf ("Unknown option in: %s\n", CMD_ERR2);
    else
        printf ("Should have been an error...\n");
    cmd_parse_free (pcmd);

    exit (EXIT_SUCCESS);
}
