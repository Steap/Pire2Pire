#ifndef CMD_H
#define CMD_H

/*
 * A command typed by a user has the following syntax :
 * name [option=value]... [arg]...
 */

struct option {
    char      *name;
    char      *value;
};

char *option_get_name  (const struct option*);
char *option_get_value (const struct option*);

struct command {
    char               *name;
    struct option      **options;
    char               **args;
};

struct command *cmd_create          (const char *s);
char           *cmd_get_name        (const struct command *c);

/*
 * Be Extra careful when using cmd_get_next_arg and cmd_get_next_option !
 * Once all options/arguments have been parsed, it returns NULL. If you apply
 * one of these functions one more time, you will be trying to access memory
 * that you are not allowed to look at...
 */
char           *cmd_get_next_arg    (const struct command *c);
struct option  *cmd_get_next_option (const struct command *c);
void            cmd_free            (struct command *c);

#endif
