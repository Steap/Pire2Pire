#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "cmd_parser.h"

static int
is_a_delimiter (char c) {
    return c == ' ' || c == '\n' || c == '\0';
}

/*
 * Gets the token at offset specified and moves the offset in front of the
 * next token, so that a subsequent call will read the next token.
 * ARGUMENTS:
 *  cmd: the command line
 *  offset: pointer to an offset
 * RETURN:
 *  A dynamically-allocated string containing the token read
 */
static char *
get_token (const char *cmd, int *offset) {
    char    *token;
    int     beginning_offset;
    int     token_size;

    /* We avoid the possibly numerous delimiters from the offset */
    while(is_a_delimiter (cmd[*offset])) {
        if (cmd[*offset] == '\0')
            return NULL;
        (*offset)++;
    }
    /* Now we're on a "real" character, so we can get the token */
    beginning_offset = *offset;
    /* Seeking the token end */
    while (!is_a_delimiter (cmd[*offset]))
        (*offset)++;

    token_size = *offset - beginning_offset;
    if (!token_size)
        return NULL;

    token = (char *)malloc ((token_size + 1) * sizeof (char));
    strncpy (token, cmd + beginning_offset, token_size);
    token[token_size] = '\0';
    /* Prepare the offset for the next command */
    if (cmd[*offset] != '\0')
        (*offset)++;

    return token;
}

/*
 * Appends an arg to the given arg_list, and returns the arg created (new last)
 */
static
struct arg_list *append_arg (struct arg_list *last, char *arg_text) {
    struct arg_list *new_arg;

    new_arg = (struct arg_list *)malloc (sizeof (struct arg_list));
    new_arg->text = arg_text;
    new_arg->next = NULL;
    if (last)
        last->next = new_arg;

    return new_arg;
}

/*
 * Frees an argument linked list properly
 */
static void
free_arg_list (struct arg_list *arg_list) {
    if (arg_list) {
        if (arg_list->next)
            free_arg_list (arg_list->next);
        free (arg_list->text);
        free (arg_list);
    }
}

/* see .h */
void
cmd_parse_free (struct parsed_cmd *parsed_cmd) {
    /* If the parse ended bad, nothing to free */
    if (!parsed_cmd
        || parsed_cmd == PARSER_MISSING_ARGUMENT
        || parsed_cmd == PARSER_UNKNOWN_OPTION)
        return;
    /* Else... */
    free (parsed_cmd->cmd);
    for (int i = 0; i < parsed_cmd->nb_template_options; i++) {
        if(parsed_cmd->options[i].value)
            free (parsed_cmd->options[i].value);
    }
    free (parsed_cmd->options);
    free_arg_list (parsed_cmd->arguments);
    free (parsed_cmd);
}

/* see .h */
struct parsed_cmd *
cmd_parse (const char *cmd, struct option_template *template) {
    struct parsed_cmd       *parsed_cmd;
    int                     cursor;
    char                    *token;
    int                     token_size;
    struct option_template  *option;
    void                    *error;
    int                     i;
    struct arg_list         *last_arg;

    error = NULL;

    cursor = 0;
    token = get_token (cmd, &cursor);
    if (!token) {
        return NULL;
    }
    parsed_cmd = (struct parsed_cmd *)malloc (sizeof (struct parsed_cmd));
    parsed_cmd->cmd = token;

    /* Reckon the number of options in the template */
    option = template;
    for (parsed_cmd->nb_template_options = 0;
            option[parsed_cmd->nb_template_options].short_name != 0;
            parsed_cmd->nb_template_options++);
    /* calloc so that each "is_in_cmd" is initialized to 0 and "value" to NULL */
    parsed_cmd->options = (struct parsed_option *)
                            calloc (parsed_cmd->nb_template_options,
                                    sizeof(struct parsed_option));
    parsed_cmd->nb_arguments = 0;
    parsed_cmd->arguments = NULL;
    last_arg = NULL;

    while ((token = get_token (cmd, &cursor))) {
        token_size = strlen (token);
        if (token[0] == '-') {
            /* "ls -" > "-" is considered an argument */
            if (token_size == 1) {
                last_arg = append_arg (last_arg, token);
                if (!parsed_cmd->arguments)
                    parsed_cmd->arguments = last_arg;
                parsed_cmd->nb_arguments++;
                continue;
            }
            if (token[1] == '-') {
                /* TODO: is -- the end of options? */
                /* for now, -- is considered an argument */
                if (token_size == 2) {
                    last_arg = append_arg (last_arg, token);
                    if (!parsed_cmd->arguments)
                        parsed_cmd->arguments = last_arg;
                    parsed_cmd->nb_arguments++;
                    continue;
                }

                /*
                 * We're dealing with a --long_name here
                 * Let's find the option in the template
                 */
                option = template;
                for (i = 0; option[i].short_name != 0; i++) {
                    /* + 2 to avoid the beginning "--" */
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
                            /* Set the option as "present" */
                            parsed_cmd->options[i].is_in_cmd = 1;
                        }
                        /*
                         * If the option requires an argument
                         */
                        if (option[i].opt_arg == ARG_REQUIRED) {
                            /* Retrieve the argument */
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
                /* If we didn't find the option in the template, error */
                if (!option[i].short_name) {
                    error = PARSER_UNKNOWN_OPTION;
                    goto parse_error;
                }
                else {
                    continue;
                }
            }

            /*
             * We're dealing with a token beginning with a - but with a long
             * name right after. This is unexpected.
             */
            if (token_size > 2) {
                error = PARSER_UNKNOWN_OPTION;
                goto parse_error;
            }

            /* We're dealing with a -short_name here */
            option = template;
            for (i = 0; option[i].short_name != 0; i++) {
                /* + 1 to avoid the beginning "-" */
                if (token[1] == option[i].short_name) {
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
                        /* Set the option as "present" */
                        parsed_cmd->options[i].is_in_cmd = 1;
                    }
                    /*
                     * If the option requires an argument
                     */
                    if (option[i].opt_arg == ARG_REQUIRED) {
                        /* Retrieve the argument */
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
            /* If we didn't find the option in the template, error */
            if (!option[i].short_name) {
                error = PARSER_UNKNOWN_OPTION;
                goto parse_error;
            }
            else {
                continue;
            }
        }
        /* We're dealing with an argument here */
        else {
            last_arg = append_arg (last_arg, token);
            if (!parsed_cmd->arguments)
                parsed_cmd->arguments = last_arg;
            parsed_cmd->nb_arguments++;
            continue;
        }
    }

    return parsed_cmd;

parse_error:
    cmd_parse_free (parsed_cmd);
    free (token);
    return error;
}

char cmd_get_next_opt (struct parsed_cmd *pcmd,
                    struct option_template *template,
                    int *option_index) {
    /* We browse the options from option_index */
    while (template[*option_index].short_name) {
        /* If it is set, we want to return its short name */
        if (pcmd->options[*option_index].is_in_cmd)
            /*
             * We get the short name from the template
             * Trick : we post-increment option_index so that next call will
             * begin from next index
             */
            return template[(*option_index)++].short_name;
        else
            (*option_index)++;
    }
    return 0;
}

char *cmd_get_opt_value (struct parsed_cmd *pcmd,
                            struct option_template *template,
                            const int *option_index) {
    /*
     * Here, we must remember that cmd_get_next_opt () has already incremented
     * option_index, so we want to check option_index - 1
     */
    /* If the option requires no argument, we return NULL */
    if (template[(*option_index) - 1].opt_arg == NO_ARG)
        return NULL;
    /* Else, we return the value at the right index */
    return pcmd->options[(*option_index) - 1].value;
}
