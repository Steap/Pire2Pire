#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>     // read()
#include <string.h>     // strcpy ()

#define BUFFER_SIZE 9

char *
prompt () {
    int     nb_read;
    char    buffer[BUFFER_SIZE];
    int     buffer_offset = 0;
    char    *line_read;

    if ((line_read = calloc (BUFFER_SIZE + 1, sizeof (char))) == NULL) {
        perror ("calloc");
        exit (EXIT_FAILURE);
    }
    printf ("> ");
    if (fflush (stdout) == EOF) {
        perror ("fflush");
        exit (EXIT_FAILURE);
    }
    // BUFFER_SIZE - 1 so we can put the terminating null byte
    while ((nb_read = read (STDIN_FILENO, buffer, BUFFER_SIZE - 1)) != 0) {
        if (nb_read < 0) {
            perror ("read");
            exit (EXIT_FAILURE);
        }
        buffer[nb_read] = '\0';

        if ((line_read = realloc (line_read,
                                  buffer_offset + BUFFER_SIZE + 1)) == NULL) {
            perror ("realloc");
            exit (EXIT_FAILURE);
        }
        strcpy (line_read + buffer_offset, buffer);
        buffer_offset += nb_read;
        line_read[buffer_offset] = '\0';

        if (line_read[buffer_offset - 1] == '\n')
            break;
    }

    line_read[buffer_offset - 1] = '\0';

    return line_read;
}
