#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#define ALL_MESSAGES    1


static pthread_mutex_t lock_log = PTHREAD_MUTEX_INITIALIZER;

static void
logger (FILE *log, const char *msg, const char *header, va_list *arg) {
    pthread_mutex_lock (&lock_log);
    fprintf (log, "%s", header);
    vfprintf (log, msg, *arg);
    fprintf (log, "\n");
    fflush (log);
    pthread_mutex_unlock (&lock_log);
}

static void
logger_no_newline (FILE *log, const char *msg,
                    const char *header, va_list *arg) {
    pthread_mutex_lock (&lock_log);
    fprintf (log, "%s", header);
    vfprintf (log, msg, *arg);
    fflush (log);
    pthread_mutex_unlock (&lock_log);
}

void
log_success (FILE *log, const char *msg, ...) {
    va_list arg;
    va_start (arg, msg);
    logger (log, msg, " [+] ", &arg);
    va_end (arg);
}

void
log_failure (FILE *log, const char *msg, ...) {
    va_list arg;
    va_start (arg, msg);
    logger (log, msg, " [FAIL] ", &arg);
    va_end (arg);
}

void
log_recv (FILE *log, const char *msg, ...) {
#if ALL_MESSAGES
    va_list arg;
    va_start (arg, msg);
    logger (log, msg, " [<] ", &arg);
    va_end (arg);
#endif
}

void
log_send (FILE *log, const char *msg, ...) {
#if ALL_MESSAGES
    va_list arg;
    va_start (arg, msg);
    logger_no_newline (log, msg, " [>] ", &arg);
    va_end (arg);
#endif
}
