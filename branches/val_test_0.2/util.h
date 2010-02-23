#ifndef UTIL_H
#define UTIL_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/*
 * Macros used to handle errors linked to system calls.
 * TODO : define more of these, with more explicit names, and with different
 * behaviours (exiting, not exiting, printing another kind of message...
 * whatever we need).
 */
#define ERROR_HANDLER(msg)     \
    {                          \
        perror (msg);          \
        exit (EXIT_FAILURE);   \
    }

/*
 * Checks whether msg begins or is cmd
 * It uses lazy evaluation to avoid accessing msg[] too far
 * TODO : maybe we could change the way it is checked...
 */
#define IS_CMD(msg,cmd) (                                   \
(strlen (msg) > strlen (cmd))                               \
&& (msg[strlen(cmd)] == ' '                                 \
    || msg[strlen(cmd)] == '\n'                             \
    || msg[strlen(cmd)] == '\r')                            \
&& (strncmp ((msg), (cmd), strlen (cmd)) == 0)              \
)

void socket_close (int fd);

#endif
