#include <stdio.h>
#include <stdlib.h>

#include "cmd_parser.h"

#define CMD_TO_PARSE "list -e foo -r caca pipi lol --exclude bar\n"

static struct option_template options[] = {
    {"e", "exclude", ARG_REQUIRED},
    {"g", "gay", ARG_REQUIRED},
    {"r", "recursive", NO_ARG},
    {NULL, NULL, 0}
};

int main () {
    struct parsed_cmd   *pcmd;
    struct arg_list     *arg;

    pcmd = cmd_parse (CMD_TO_PARSE, options);
    if (!pcmd)
        exit (EXIT_FAILURE);

    printf ("Command: %s\n", pcmd->cmd);
    printf ("Options:\n");
    for (int i = 0; options[i].short_name != NULL; i++) {
        printf ("--%-20s (-%s) is %s",
                options[i].long_name,
                options[i].short_name,
                (pcmd->options[i].is_in_cmd) ? "set" : "not set");
        if (pcmd->options[i].is_in_cmd &&
            options[i].opt_arg == ARG_REQUIRED) {
            printf (" and its value is %s", pcmd->options[i].value);
        }
        printf ("\n");
    }
    printf ("Arguments:\n");
    arg = pcmd->arguments;
    while (arg) {
        printf ("%s\n", arg->text);
        arg = arg->next;
    }

    parsed_cmd_free (pcmd);

    exit (EXIT_SUCCESS);
}
