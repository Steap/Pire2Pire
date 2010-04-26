#ifndef CMD_PARSER_H
#define CMD_PARSER_H

#define PARSER_MISSING_ARGUMENT     (struct parsed_cmd *)-1
#define PARSER_UNKNOWN_OPTION       (struct parsed_cmd *)-2
#define PARSER_EMPTY_COMMAND        (struct parsed_cmd *)-3

// Define whether an argument is needed for an option
enum opt_arg {
    ARG_REQUIRED,   // Argument required
    NO_ARG          // No argument
};

// Define a template to parse a command line
struct option_template {
    char            short_name;     // e.g. 'r' for -r
    char            *long_name;     // e.g. "recursive" for --recursive
    enum opt_arg    opt_arg;        // see above
};

// There is one of these structs by option in the template
struct parsed_option {
    // Boolean: is the option set in the parsed command
    int     is_in_cmd;
    // What's its value?
    char    *value;
};

// Return of the function cmd_parse ()
struct parsed_cmd {
    // The number of options IN THE TEMPLATE, useful for free-ing this struct
    int                     nb_template_options;
    // An array with one parsed_option by non-NULL element in the template
    struct parsed_option    *options;
    // The number of arguments passed in the command line (+ 1 for command)
    int                     argc;
    // Similar to well-known argv, omitting all options and their value
    char              **argv;
};

/*
 * This function parses the command line cmd according to the template, and
 * returns a newly allocated parsed_cmd, that should be freed with
 * parsed_cmd_free () once you are done with it.
 */
struct parsed_cmd *cmd_parse (const char *cmd,
                                const struct option_template *template);

/*
 * This function takes the return of cmd_parse and tells if the parse failed
 * RETURN VALUE:
 * 0 if the parse failed, 1 otherwise
 */
int cmd_parse_failed (const struct parsed_cmd *pcmd);

/*
 * This frees a struct parsed_cmd created by cmd_parse ()
 */
void cmd_parse_free (struct parsed_cmd *parsed_cmd);

/*
 * This returns the next option set from option_index in pcmd, according to
 * template. Returns 0 when no option left.
 */
char cmd_get_next_opt (const struct parsed_cmd *pcmd,
                        const struct option_template *template,
                        int *option_index);

/*
 * This function MUST BE CALLED after a cmd_get_next_opt () call, in order
 * to retrieve (if it exists) the value of the option
 */
char *cmd_get_opt_value (const struct parsed_cmd *pcmd,
                            const struct option_template *template,
                            const int *option_index);

#endif
