#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#define FOO 0

void
log_success (FILE *log, const char *msg, ...) {
    va_list arg;
    va_start (arg, msg);
    fprintf (log, " [+] ");
    vfprintf (log, msg, arg);
    fprintf (log, "\n");
    fflush (log);
    va_end (arg);
}

void
log_failure (FILE *log, char *msg, ...) {
    va_list arg;
    va_start (arg, msg);
    fprintf (log, " [FAIL] ");
    vfprintf (log, msg, arg);
    fprintf (log, "\n");
    fflush (log);
    va_end (arg);
}
