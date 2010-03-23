#ifndef CMD_PARSER_H
#define CMD_PARSER_H

#define PARSER_MISSING_ARGUMENT     -1
#define PARSER_UNKNOWN_OPTION       -2

// Define whether an argument is needed for an option
enum opt_arg {
    ARG_REQUIRED,   // Argument required
    NO_ARG          // No argument
};

// Define a template to parse a command line
struct option_template {
    char            *short_name;    // e.g. "r" for -r
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

// A simple linked list of arguments given to the command line
struct arg_list {
    char                *text;
    struct arg_list     *next;
};

// Return of the function cmd_parse ()
struct parsed_cmd {
    char                    *cmd;           // The command (first token)
    // The number of options in the template, useful for free-ing this struct
    int                     nb_options;
    // An array with one parsed_option by non-NULL element in the template
    struct parsed_option    *options;
    // The number of arguments passed in the command line
    int                     nb_arguments;
    // Linked list of arguments
    struct arg_list         *arguments;
};

/*
 * This function parses the command line cmd according to the template, and
 * returns a newly allocated parsed_cmd, that should be freed with
 * parsed_cmd_free () once you are done with it.
 */
struct parsed_cmd *cmd_parse (const char *cmd,
                                struct option_template *template);

/*
 * This frees a struct parsed_cmd created by cmd_parse ()
 */
void parsed_cmd_free (struct parsed_cmd *parsed_cmd);

#endif
