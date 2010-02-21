#ifndef UTIL_H
#define UTIL_H

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

void socket_close (int fd);

#endif
