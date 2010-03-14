#include <sys/ioctl.h>      // TIOCSTI, not POSIX :(
#include <sys/select.h>     // timeval

#include <errno.h>
#include <signal.h>         // sigaction
#include <stdlib.h>         // NULL
#include <stdio.h>          // perror ()
#include <string.h>         // strcpy ()
#include <stropts.h>        // ioctl ()
#include <termios.h>        // termios
#include <unistd.h>         // STDIN_FILENO

#define BUFFER_SIZE 128

/*
 * This function is called when the thread is interrupted by SIGUSR1,
 * thus indicating that something was written on the tty. If the user
 * was typing something at the time, it would be a good idea to display
 * him his previously inputed characters again, so that he won't be lost
 * and can backspace them. That's what we will do quite subtly...
 */
static void
paste () {
    struct termios      tty;
    struct timeval      timeout;
    int                 select_return;
    fd_set              set;
    char                *line_read;
    char                buffer[BUFFER_SIZE];
    int                 buffer_offset;
    int                 nb_read;
    struct sigaction    old_action;


    // We don't want to be interrupted within this interruption handling.
    sigaction (SIGUSR1, NULL, &old_action);

    // We will read what the user was typing.
    line_read = NULL;
    buffer_offset = 0;

    /*
     * Here, we put the tty in RAW mode (instead of CANONICAL mode), in this
     * mode, it will send characters one by one instead of waiting for the user
     * to write a full line. Basically, stdin will be filled with anything he
     * has begun to type.
     */
    tcgetattr (STDIN_FILENO, &tty);
    tty.c_lflag &= ~ICANON;
    tcsetattr (STDIN_FILENO, TCSANOW, &tty);

    /*
     * Now we will try to read stdin, but how do we know we're done ? Let's
     * say that if nothing is in stdin after a few milliseconds, then we are.
     */
    timeout.tv_sec = 0;
    timeout.tv_usec = 100;

    FD_ZERO (&set);
    for (;;) {
        // It is vital to set the set at each loop, because select alters it
        FD_SET (STDIN_FILENO, &set);
        // select will either tell us stdin is filled or timeout
        select_return = select (STDIN_FILENO + 1, &set, NULL, NULL, &timeout);
        if (select_return < 0) {
            perror ("select");
            exit (EXIT_FAILURE);
        }
        // select_return > 0 => stdin contains a character
        else if (select_return) {
            nb_read = read (STDIN_FILENO, buffer, BUFFER_SIZE - 1);
            if (nb_read < 0) {
                perror ("read");
                exit (EXIT_FAILURE);
            }
            buffer[nb_read] = '\0';

            // + 1 so we can strcpy the terminating null byte too
            if ((line_read = realloc (line_read,
                                      buffer_offset + nb_read + 1)) == NULL) {
                perror ("realloc");
                exit (EXIT_FAILURE);
            }

            strcpy (line_read + buffer_offset, buffer);
            buffer_offset += nb_read;

            line_read[buffer_offset] = '\0';
        }
        // select_return == 0 => we are done reading user input
        else
            break;
    }

    // Going back into canonical mode
    tty.c_lflag |= ICANON;
    tcsetattr (STDIN_FILENO, TCSANOW, &tty);

    // Now, we will simulate the fact that the user typed the same input again.
    // If he typed anything, obviously...
    if (line_read) {
        buffer_offset = 0;
        while (line_read[buffer_offset] != '\0') {
            // This simulates the user typing one character
            ioctl (STDIN_FILENO, TIOCSTI, line_read + buffer_offset);
            buffer_offset++;
        }
    }

    // Now we listen to SIGUSR1 signals again
    sigaction (SIGUSR1, &old_action, NULL);
}

char *
prompt () {
    int     nb_read;
    char    buffer[BUFFER_SIZE];
    int     buffer_offset = 0;
    struct sigaction    paste_action;
    char    *line_read;

    // THIS IS VERY IMPORTANT for next realloc ()
    line_read = NULL;

    sigemptyset(&paste_action.sa_mask);
    paste_action.sa_handler = paste;
    paste_action.sa_flags = 0;
    // We expect a SIGUSR1 from recv_resp thread
    if (sigaction (SIGUSR1, &paste_action, NULL) < 0) {
        perror ("sigaction");
        exit (EXIT_FAILURE);
    }

    // BUFFER_SIZE - 1 so we can put the terminating null byte
    while ((nb_read = read (STDIN_FILENO, buffer, BUFFER_SIZE - 1)) != 0) {
        if (nb_read < 0) {
            // If read was interrupted by a signal (SIGUSR1 for instance)
            if (errno == EINTR) {
                // We continue reading
                continue;
            }
            else {
                perror ("read");
                exit (EXIT_FAILURE);
            }
        }
        buffer[nb_read] = '\0';

        // + 1 so we can strcpy the terminating null byte too
        if ((line_read = realloc (line_read,
                                  buffer_offset + nb_read + 1)) == NULL) {
            perror ("realloc");
            exit (EXIT_FAILURE);
        }

        strcpy (line_read + buffer_offset, buffer);
        buffer_offset += nb_read;

        line_read[buffer_offset] = '\0';

        if (line_read[buffer_offset - 1] == '\n')
            break;
    }

    return line_read;
}
