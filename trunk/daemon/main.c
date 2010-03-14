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
#include <semaphore.h>

#include <arpa/inet.h>
#include <netinet/in.h>

#include "../util/logger.h"
#include "util/socket.h"
#include "conf.h"
#include "client_handler.h"
#include "daemon_handler.h"

#define LOG_FILE        "/tmp/g"
#define LOCK_FILE       "/tmp/k"
// nfds must be the maximum sd + 1
#define NFDS(a,b)   (((a)>(b))?(a+1):(b+1))

FILE *log_file;

/* Preferences defined in daemon.conf. */
struct prefs *prefs;

/* The lists lock that must be init on startup */
extern sem_t    clients_lock;
extern sem_t    daemons_lock;

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
        if (log_file)
            log_failure (log_file, "Could not destroy the lock file");
    if (log_file) {
        log_success (log_file, "Stopping server, waiting for SIGKILL");    
        fclose (log_file);
    }
    exit (0);
}

void daemonize(void) {
    int lock;
    char str[10];

    switch (fork ()) {
        case -1:    
            exit (1);
        case 0:
            exit (0);
            break;
        default:
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

    sigset_t mask;
    sigemptyset (&mask);
    sigaddset (&mask, SIGCHLD);
    sigaddset (&mask, SIGTSTP);
    sigaddset (&mask, SIGTTOU);
    sigaddset (&mask, SIGTTIN);
    sigprocmask (SIG_BLOCK, &mask, NULL);

    struct sigaction    on_signal;
    on_signal.sa_flags = 0;
    on_signal.sa_handler = signal_handler;
    sigaction (SIGHUP, &on_signal, NULL);
    on_signal.sa_handler = server_stop;
    sigaction (SIGINT, &on_signal, NULL);
    sigaction (SIGTERM, &on_signal, NULL);

    log_success (log_file, "Signals deferred or handled.");

}

static void
start_server (void) {
    int     client_sd;
    int     daemon_sd;
    int     connected_sd;
    struct sockaddr_in  client_sa;
    struct sockaddr_in  daemon_sa;
    struct sockaddr_in  connected_sa;
    socklen_t size = sizeof (struct sockaddr);
    fd_set  sockets;
    int nfds;

    /* Initializing the global semaphores */
    if (sem_init (&clients_lock, 0, 1) < 0) {
        log_failure (log_file, "Unable to sem_init clients_lock");
        exit (EXIT_FAILURE);
    }
    if (sem_init (&daemons_lock, 0, 1) < 0) {
        log_failure (log_file, "Unable to sem_init daemons_lock");
        exit (EXIT_FAILURE);
    }

    client_sa.sin_family        = AF_INET;
    client_sa.sin_addr.s_addr   = INADDR_ANY;
    client_sa.sin_port          = htons (prefs->client_port);
    client_sd = socket_init (&client_sa);
    if (client_sd < 0) {
        exit (EXIT_FAILURE);
    }

    daemon_sa.sin_family        = AF_INET;
    daemon_sa.sin_addr.s_addr   = INADDR_ANY;
    daemon_sa.sin_port          = htons (prefs->daemon_port);
    daemon_sd = socket_init (&daemon_sa);
    if (daemon_sd < 0) {
        exit (EXIT_FAILURE);
    }

    // sockets contains both client_sd and daemon_sd
    FD_ZERO (&sockets);
    nfds = NFDS (client_sd, daemon_sd);
    for (;;) {
        /*
         * It is VERY important to FD_SET at each loop, because select
         * will FD_UNSET the socket descriptors
         */
        FD_SET (client_sd, &sockets);
        FD_SET (daemon_sd, &sockets);

        // Block until a socket is ready to accept
        if (select (nfds, &sockets, NULL, NULL, NULL) < 0) {
            log_failure (log_file, "main () : select failed");
        }

        if (FD_ISSET (client_sd, &sockets)) {
            if ((connected_sd = (accept (client_sd,
                                        (struct sockaddr *) &connected_sa,
                                        &size))) < 0)
                log_failure (log_file, "Failed to accept incoming connection.");
            handle_client (connected_sd, &connected_sa);
        }
        else if (FD_ISSET (daemon_sd, &sockets)) {
            if ((connected_sd = (accept (daemon_sd,
                                        (struct sockaddr *) &connected_sa,
                                        &size))) < 0)
                log_failure (log_file, "Failed to accept incoming connection.");
            handle_daemon (connected_sd, &connected_sa);
        }
        else {
            // This should never happen : neither client nor daemon!?
            log_failure (log_file, "Unknown connection");
        }
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
