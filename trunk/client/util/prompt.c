#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>     // read()
#include <string.h>     // strcpy ()
#include <signal.h>     // sigaction, sigaction (), sigemptyset ()
#include <pthread.h>
#include <errno.h>
#include <termios.h>
#include <sys/ioctl.h>

#define BUFFER_SIZE 9

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

    sigaction (SIGUSR1, NULL, &old_action);

    line_read = NULL;
    buffer_offset = 0;

    tcgetattr (STDIN_FILENO, &tty);
    tty.c_lflag &= ~ICANON;
    tcsetattr (STDIN_FILENO, TCSANOW, &tty);
    timeout.tv_sec = 0;
    timeout.tv_usec = 100;

    FD_ZERO (&set);
    for (;;) {
        FD_SET (STDIN_FILENO, &set);
        select_return = select (STDIN_FILENO + 1, &set, NULL, NULL, &timeout);
        if (select_return < 0) {
            perror ("select");
            exit (EXIT_FAILURE);
        }
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
        else
            break;
    }

    tty.c_lflag |= ICANON;
    tcsetattr (STDIN_FILENO, TCSANOW, &tty);

    if (line_read) {
        buffer_offset = 0;
        while (line_read[buffer_offset] != '\0') {
            ioctl (STDIN_FILENO, TIOCSTI, line_read + buffer_offset);
            buffer_offset++;
        }
    }

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
            if (errno == EINTR) {
                continue;
            }
            else {
                perror ("read");
            }
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

        if (line_read[buffer_offset - 1] == '\n')
            break;
    }

    return line_read;
}
