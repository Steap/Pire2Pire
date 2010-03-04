//http://www.enderunix.org/docs/eng/exampled.c
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "../util/logger.h"

#define LOG_FILE        "/tmp/g"
#define LOCK_FILE       "/tmp/k"

static FILE *log;

static void
signal_handler (int sig) {
    (void) sig;
    if (log)
    {    
        fprintf (log, "Receveid signal !\n");
        fflush (log);
    }
}

void daemonize()
{
    int lock;
    char str[10];

    switch (fork ())
    {
        case -1:    
            exit (1);
        case 0:
            break;
        default:
            exit (0);
            break;
    }
    /*
     * We should certainly do that, but there will be many bad file descriptors,
     * which definitely sucks ass.
     * FIXME : find a clever way to close all fds ?
     */
#if 0
    for (i = 0; i < getdtablesize (); i--)
        if (close (i) == -1)
            perror ("close");
#endif
    log = fopen (LOG_FILE, "w");

    setsid ();
    log_success (log, "setsid ok");

    umask (027);
    log_success (log, "Set file permissions to 750.");

    lock = open (LOCK_FILE, O_RDWR | O_CREAT, 0640);
    if (lock < 0)
        log_failure (log, "Failed to open lock file (%s).", LOCK_FILE);
    else
        log_success (log, "Opened lock file (%s).", LOCK_FILE);
    sprintf (str, "%d\n", getpid ());
    write (lock, str, strlen (str));
    if (close (lock) < 0)
        log_failure (log, "Failed to close LOCK_FILE (%s).", LOCK_FILE);
    else
        log_success (log, "Created lock file (%s)", LOCK_FILE);


    signal (SIGCHLD, SIG_IGN);
    log_success (log, "Ignoring SIGCHLD");
    signal (SIGTSTP, SIG_IGN);
    log_success (log, "Ignoring SIGTSTP");
    signal (SIGTTOU, SIG_IGN);
    log_success (log, "Ignoring SIGTTOU");
    signal (SIGTTIN, SIG_IGN);
    log_success (log, "Ignoring SIGTTIN");

    signal (SIGHUP, signal_handler); 
    log_success (log, "Connected signal handler.");

}
int
main ()
{
    daemonize ();
    for (;;)
        sleep (1);   
    fprintf (stderr, "LOL");
    
    return EXIT_SUCCESS;
}
