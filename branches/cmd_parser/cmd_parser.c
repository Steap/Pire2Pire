#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "cmd_parser.h"

static int
is_a_delimiter (char c) {
    return c == ' ' || c == '\n' || c == '\0';
}

static char *
get_token (const char *cmd, int *offset) {
    char    *token;
    int     beginning_offset;
    int     token_size;

    beginning_offset = *offset;
    while (!is_a_delimiter (cmd[*offset]))
        (*offset)++;

    token_size = *offset - beginning_offset;
    if (!token_size)
        return NULL;

    token = (char *)malloc ((token_size + 1) * sizeof (char));
    strncpy (token, cmd + beginning_offset, token_size);
    token[token_size] = '\0';

    // We prepare the offset for the next command
    if (cmd[*offset] != '\0')
        (*offset)++;

    return token;
}

static
struct arg_list *add_arg (struct arg_list *arg_list, char *arg_text) {
    struct arg_list *new_arg;

    new_arg = (struct arg_list *)malloc (sizeof (struct arg_list));
    new_arg->text = arg_text;
    new_arg->next = arg_list;

    return new_arg;
}

static void
free_arg_list (struct arg_list *arg_list) {
    if (arg_list) {
        if (arg_list->next)
            free_arg_list (arg_list->next);
        free (arg_list->text);
        free (arg_list);
    }
}

void
parsed_cmd_free (struct parsed_cmd *parsed_cmd) {
    free (parsed_cmd->cmd);
    for (int i = 0; i < parsed_cmd->nb_options; i++) {
        if(parsed_cmd->options[i].value)
            free (parsed_cmd->options[i].value);
    }
    free (parsed_cmd->options);
    free_arg_list (parsed_cmd->arguments);
    free (parsed_cmd);
}

struct parsed_cmd *
cmd_parse (const char *cmd, struct option_template *template) {
    struct parsed_cmd       *parsed_cmd;
    int                     cursor;
    char                    *token;
    int                     token_size;
    struct option_template  *option;
    int                     error;
    int                     i;

    cursor = 0;
    token = get_token (cmd, &cursor);
    if (!token) {
        return NULL;
    }
    parsed_cmd = (struct parsed_cmd *)malloc (sizeof (struct parsed_cmd));
    parsed_cmd->cmd = token;

    // Reckon the number of options in the template
    option = template;
    for (parsed_cmd->nb_options = 0;
            option[parsed_cmd->nb_options].short_name != NULL;
            parsed_cmd->nb_options++);
    // calloc so that each "is_in_cmd" is initialized to 0 and "value" to NULL
    parsed_cmd->options = (struct parsed_option *)
                            calloc (parsed_cmd->nb_options,
                                    sizeof(struct parsed_option));
    parsed_cmd->nb_arguments = 0;
    parsed_cmd->arguments = NULL;

    while ((token = get_token (cmd, &cursor))) {
        token_size = strlen (token);
        if (token[0] == '-') {
            // "ls -" > "-" is considered an argument
            if (token_size == 1) {
                parsed_cmd->arguments = add_arg (parsed_cmd->arguments, token);
                parsed_cmd->nb_arguments++;
                continue;
            }
            if (token[1] == '-') {
                // TODO: is -- the end of options?
                // for now, -- is considered an argument
                if (token_size == 2) {
                    parsed_cmd->arguments = add_arg (parsed_cmd->arguments,
                                                        token);
                    parsed_cmd->nb_arguments++;
                    continue;
                }

                // We're dealing with a --long_name here
                // Let's find the option in the template
                option = template;
                for (i = 0; option[i].short_name != NULL; i++) {
                    // + 2 to avoid the beginning "--"
                    if (strcmp (token + 2, option[i].long_name) == 0) {
                        /*
                         * We've found what option we're dealing with so
                         * let's free token
                         */
                        free (token);
                        /*
                         * If the option is set twice or more, we keep the last
                         * occurence as a reference
                         */
                        if (parsed_cmd->options[i].is_in_cmd
                            && parsed_cmd->options[i].value) {
                            free (parsed_cmd->options[i].value);
                            parsed_cmd->options[i].value = NULL;
                        }
                        else {
                            // Set the option as "present"
                            parsed_cmd->options[i].is_in_cmd = 1;
                        }
                        /*
                         * If the option requires an argument
                         */
                        if (option[i].opt_arg == ARG_REQUIRED) {
                            // Retrieve the argument
                            token = get_token (cmd, &cursor);
                            if (!token) {
                                error = PARSER_MISSING_ARGUMENT;
                                goto parse_error; 
                            }
                            parsed_cmd->options[i].value = token;
                        }
                        break;
                    }
                }
                // If we didn't find the option in the template, error
                if (!option[i].short_name) {
                    error = PARSER_UNKNOWN_OPTION;
                    goto parse_error;
                }
                else {
                    continue;
                }
            }

            // We're dealing with a -short_name here
            option = template;
            for (i = 0; option[i].short_name != NULL; i++) {
                // + 1 to avoid the beginning "-"
                if (strcmp (token + 1, option[i].short_name) == 0) {
                    /*
                     * We've found what option we're dealing with so
                     * let's free token
                     */
                    free (token);
                    /*
                     * If the option is set twice or more, we keep the last
                     * occurence as a reference
                     */
                    if (parsed_cmd->options[i].is_in_cmd
                        && parsed_cmd->options[i].value) {
                        free (parsed_cmd->options[i].value);
                        parsed_cmd->options[i].value = NULL;
                    }
                    else {
                        // Set the option as "present"
                        parsed_cmd->options[i].is_in_cmd = 1;
                    }
                    /*
                     * If the option requires an argument
                     */
                    if (option[i].opt_arg == ARG_REQUIRED) {
                        // Retrieve the argument
                        token = get_token (cmd, &cursor);
                        if (!token) {
                            error = PARSER_MISSING_ARGUMENT;
                            goto parse_error; 
                        }
                        parsed_cmd->options[i].value = token;
                    }
                    break;
                }
            }
            // If we didn't find the option in the template, error
            if (!option[i].short_name) {
                error = PARSER_UNKNOWN_OPTION;
                goto parse_error;
            }
            else {
                continue;
            }
        }
        // We're dealing with an argument here
        else {
            parsed_cmd->arguments = add_arg (parsed_cmd->arguments, token);
            parsed_cmd->nb_arguments++;
            continue;
        }
    }

    return parsed_cmd;

parse_error:
    switch (error) {
        case PARSER_MISSING_ARGUMENT:
            printf ("Error: Missing argument for option --%s (-%s)\n",
                    option[i].long_name, option[i].short_name);
            break;
        case PARSER_UNKNOWN_OPTION:
            printf ("Error: Unknown option %s\n", token);
            break;
        default:
            printf ("Unknown error\n");
            break;
    }
    parsed_cmd_free (parsed_cmd);
    free (token);
    return NULL;
}

