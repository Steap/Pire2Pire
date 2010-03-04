//http://www.enderunix.org/docs/eng/exampled.c
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <netinet/in.h>

#include <errno.h>
#include "../util/logger.h"

#define LOG_FILE        "/tmp/g"
#define LOCK_FILE       "/tmp/k"

static FILE *log;


/* Rename, plz... Or not, it is just  a silly test */
static void
signal_handler (int sig) {
    (void) sig;
    if (log)
    {    
        fprintf (log, "Received signal !\n");
        fflush (log);
    }
}

static void 
server_stop (int sig) {
    (void) sig;
    if (unlink (LOCK_FILE) < 0)
        log_failure (log, "Could not destroy the lock file");
    log_success (log, "Stopping server, waiting for SIGKILL");    
}

void daemonize(void) {
    int lock;
    char str[10];

    switch (fork ()) {
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

    if (lockf (lock, F_TLOCK, 0) < 0) {
        log_failure (log, "Could not lock %s", LOCK_FILE);
    }

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
    signal (SIGTERM, server_stop);

    log_success (log, "Connected signal handler.");

}

#define PORT 1339
#define NB_QUEUE 10
static void
start_server (void) {
    int yes =1;
    int sd;
    struct sockaddr_in sa;

    if ((sd = socket (AF_INET, SOCK_STREAM, 0)) < 0) {
        log_failure (log, "Socket creation failed.");
        exit (1);
    }
    else {
        log_success (log, "Created socket.");
    }

    sa.sin_family        = AF_INET;
    sa.sin_addr.s_addr   = INADDR_ANY;
    sa.sin_port          = htons (PORT);

    if (setsockopt (sd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof (int)) < 0) {
        log_failure (log, "Setsockopt failed.");
    }

    if (bind (sd, (struct sockaddr *) &sa, sizeof (sa)) < 0) {
        log_failure (log, "Could not assign a local address using bind %s.", errno);
        exit (1);
    }
     
    if (listen (sd, NB_QUEUE) < 0) {
        log_failure (log, "The socket could not be marked as a passive one.");
        exit (1);
    }
    else {
        log_success (log, "Server is ready.");
    }

    for (;;) {
        if (accept (sd, NULL, NULL) < 0) {
            log_failure (log, "Failed to accept incoming connection.");
            exit (EXIT_FAILURE);
        } 
        switch (fork ()) {
            case -1:
                log_failure (log, "Could not fork !");
                break;
            case 0:
                log_success (log, "Should handle client");
            default:
                break;
        }
    } 
}

int
main ()
{
    daemonize ();
    start_server ();
    for (;;)
        sleep (1);   
    fprintf (stderr, "LOL");
    
    return EXIT_SUCCESS;
}
