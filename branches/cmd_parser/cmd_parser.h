#ifndef CMD_PARSER_H
#define CMD_PARSER_H

#define PARSER_MISSING_ARGUMENT     -1
#define PARSER_UNKNOWN_OPTION       -2

enum opt_arg {
    ARG_REQUIRED,
    NO_ARG
};

struct option_template {
    char            *short_name;
    char            *long_name;
    enum opt_arg    opt_arg;
};

struct parsed_option {
    // Boolean: is the option set in the parsed command
    int     is_in_cmd;
    // What's its value?
    char    *value;
};

struct arg_list {
    char                *text;
    struct arg_list     *next;
};

struct parsed_cmd {
    char                    *cmd;
    int                     nb_options;
    struct parsed_option    *options;
    int                     nb_arguments;
    struct arg_list         *arguments;
};

struct parsed_cmd *cmd_parse (const char *cmd,
                                struct option_template *template);

void parsed_cmd_free (struct parsed_cmd *parsed_cmd);

#endif
