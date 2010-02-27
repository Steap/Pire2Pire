#include <stdlib.h>
#include <stdio.h>

// FIXME: return value instead of exit...
void logger (FILE *log_file, const char *msg) {
    if (fprintf(log_file, "%s", msg) < 0) {
        fprintf(stderr, "fprintf");
        exit (EXIT_FAILURE);
    }
    if (fflush (log_file) == EOF) {
        perror ("fflush");
        exit (EXIT_FAILURE);
    }
}

