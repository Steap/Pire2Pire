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
#include "conf.h"
#include "client_handler.h"

#define LOG_FILE        "/tmp/g"
#define LOCK_FILE       "/tmp/k"

FILE *log_file;

/* Preferences defined in daemon.conf. */
struct prefs *prefs;


/* Rename, plz... Or not, it is just  a silly test */
static void
signal_handler (int sig) {
    (void) sig;
    if (log_file)
    {    
        fprintf (log_file, "Received signal !\n");
        fflush (log_file);
    }
}

static void 
server_stop (int sig) {
    (void) sig;
    if (unlink (LOCK_FILE) < 0)
        log_failure (log_file, "Could not destroy the lock file");
    log_success (log_file, "Stopping server, waiting for SIGKILL");    
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
    log_file = fopen (LOG_FILE, "w");

    setsid ();
    log_success (log_file, "setsid ok");

    close (STDIN_FILENO);
    close (STDOUT_FILENO);
    close (STDERR_FILENO);
    log_success (log_file, "Closed stdin, stdout, stderr.");

    umask (027);
    log_success (log_file, "Set file permissions to 750.");

    lock = open (LOCK_FILE, O_RDWR | O_CREAT, 0640);
    if (lock < 0)
        log_failure (log_file, "Failed to open lock file (%s).", LOCK_FILE);
    else
        log_success (log_file, "Opened lock file (%s).", LOCK_FILE);

    if (lockf (lock, F_TLOCK, 0) < 0) {
        log_failure (log_file, "Could not lock %s", LOCK_FILE);
    }

    sprintf (str, "%d\n", getpid ());
    write (lock, str, strlen (str));
    if (close (lock) < 0)
        log_failure (log_file, "Failed to close LOCK_FILE (%s).", LOCK_FILE);
    else
        log_success (log_file, "Created lock file (%s)", LOCK_FILE);


    signal (SIGCHLD, SIG_IGN);
    log_success (log_file, "Ignoring SIGCHLD");
    signal (SIGTSTP, SIG_IGN);
    log_success (log_file, "Ignoring SIGTSTP");
    signal (SIGTTOU, SIG_IGN);
    log_success (log_file, "Ignoring SIGTTOU");
    signal (SIGTTIN, SIG_IGN);
    log_success (log_file, "Ignoring SIGTTIN");

    signal (SIGHUP, signal_handler); 
    signal (SIGTERM, server_stop);

    log_success (log_file, "Connected signal handler.");

}

#define NB_QUEUE 10

static void
start_server (void) {
    int yes =1;
    int sd;
    struct sockaddr_in sa;

    if ((sd = socket (AF_INET, SOCK_STREAM, 0)) < 0) {
        log_failure (log_file, "Socket creation failed.");
        exit (1);
    }
    else {
        log_success (log_file, "Created socket.");
    }

    sa.sin_family        = AF_INET;
    sa.sin_addr.s_addr   = INADDR_ANY;
    sa.sin_port          = htons (prefs->port);

    if (setsockopt (sd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof (int)) < 0) {
        log_failure (log_file, "Setsockopt failed.");
    }

    if (bind (sd, (struct sockaddr *) &sa, sizeof (sa)) < 0) {
        log_failure (log_file, "Could not assign a local address using bind %s.", errno);
        exit (1);
    }
     
    if (listen (sd, NB_QUEUE) < 0) {
        log_failure (log_file, "The socket could not be marked as a passive one.");
        exit (1);
    }
    else {
        log_success (log_file, "Server is ready (port : %d).", prefs->port);
    }

    for (;;) {
        if (accept (sd, NULL, NULL) < 0) {
            log_failure (log_file, "Failed to accept incoming connection.");
            //exit (EXIT_FAILURE);
        } 
#if 0
        switch (fork ()) {
            case -1:
                log_failure (log_file, "Could not fork !");
                break;
            case 0:
                log_success (log_file, "Should handle client");
                handle_request ("fake_cmd");
                log_success (log_file, "xxShould handle client");
            default:
                break;
        }
#endif
        handle_request ("fake");
    } 
}

int
main (int argc, char *argv[])
{
    (void )argc;
    prefs = conf_retrieve (argv[1]);
    daemonize ();
    start_server ();
    for (;;)
        sleep (1);   
    
    return EXIT_SUCCESS;
}
