#ifndef STRING_UTIL_H
#define STRING_UTIL_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/*
 * Checks whether msg begins or is cmd
 * It uses lazy evaluation to avoid accessing msg[] too far
 * TODO : maybe we could change the way it is checked...
 */
#define IS_CMD(msg,cmd) (                                   \
(strlen (msg) > strlen (cmd))                               \
&& (msg[strlen (cmd)] == ' '                                \
    || msg[strlen (cmd)] == '\n'                            \
    || msg[strlen (cmd)] == '\r')                           \
&& (strncmp ((msg), (cmd), strlen (cmd)) == 0)              \
)

/*
 * Replaces first '\r' or '\n' by a '\0'
 */
void string_remove_trailer (char *msg);

#endif
