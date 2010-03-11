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
    char    *line_read = NULL;

/* No prompt seems better actually...
    printf ("> ");
    if (fflush (stdout) == EOF) {
        perror ("fflush");
        exit (EXIT_FAILURE);
    }
*/
    // BUFFER_SIZE - 1 so we can put the terminating null byte
    while ((nb_read = read (STDIN_FILENO, buffer, BUFFER_SIZE - 1)) != 0) {
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

        if (line_read[buffer_offset - 1] == '\n')
            break;
    }

    return line_read;
}
