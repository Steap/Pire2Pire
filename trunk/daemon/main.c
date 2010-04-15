#include <errno.h>
#include <sys/ioctl.h>      // ioctl ()
#include <sys/stat.h>       // umask ()

#include <arpa/inet.h>      // inet_ntop ()
#include <net/if.h>         // struct ifreq
#include <netinet/in.h>     // struct sockaddr_in

#include <fcntl.h>          // open ()
#include <semaphore.h>      // sem_t
#include <signal.h>         // sigemptyset ()
#include <stdio.h>          // FILE
#include <string.h>         // strlen ()
#include <unistd.h>         // unlink ()

#include "../util/logger.h" // log_failure ()
#include "client_handler.h" // handle_daemon ()
#include "conf.h"           // struct prefs
#include "daemon_handler.h" // handle_client ()
#include "dl_file.h"
#include "file_cache.h"     // struct file_cache
#include "util/socket.h"    // socket_init ()

#define LOG_FILE        "/tmp/g"
#define LOCK_FILE       "/tmp/k"
// nfds must be the maximum sd + 1
#define NFDS(a,b)   (((a)>(b))?(a+1):(b+1))

FILE *log_file;
char my_ip[INET_ADDRSTRLEN];

/* Preferences defined in daemon.conf. */
struct prefs *prefs;

/* The lists lock that must be init on startup */
extern sem_t            clients_lock;
extern struct client    *clients;
extern sem_t            daemons_lock;
extern struct daemon    *daemons;

sem_t                   downloads_lock;
struct dl_file          *downloads;

/* Only one client can do list at a time */
struct client       *list_client;
sem_t               list_lock;

// Handler of known files and its semaphor
struct file_cache   *file_cache;
sem_t               file_cache_lock;

static int create_dir (const char *path, mode_t mode);

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
    struct client   *c;
    struct daemon   *d;

    (void) sig;

    sem_destroy (&clients_lock);
    sem_destroy (&daemons_lock);
    sem_destroy (&file_cache_lock);
    sem_destroy (&list_lock);
    sem_destroy (&downloads_lock);


    if (clients) {
        while (clients) {
            c = clients->next;
            free (clients);
            clients = c;
        }
        log_success (log_file, 
                     "%s : all clients have been freed",
                     __func__);
    }

    if (daemons) {
        while (daemons) {
            d = daemons->next;
            daemon_send (daemons, "quit\n");
            daemon_free (daemons);
            daemons = d;
        }
        log_success (log_file,
                     "%s : all daemons have been freed",
                     __func__);
    }

    if (file_cache) {
        file_cache_free (file_cache);
        log_success (log_file,
                     "%s : file cache has been freed",
                     __func__);
    }

    if (unlink (LOCK_FILE) < 0)
        if (log_file)
            log_failure (log_file, "Could not destroy the lock file");
    if (log_file) {
        log_success (log_file, "Stopping server, waiting for SIGKILL");
        fclose (log_file);
    }
    if (prefs) {
        conf_free (prefs);
        log_success (log_file, 
                     "%s : Preferences have been freed.",
                     __func__);
    }

    exit (EXIT_SUCCESS);
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
    sigemptyset (&on_signal.sa_mask);
    on_signal.sa_flags = 0;
    on_signal.sa_handler = signal_handler;
    sigaction (SIGHUP, &on_signal, NULL);
    on_signal.sa_handler = server_stop;
    sigaction (SIGINT, &on_signal, NULL);
    sigaction (SIGTERM, &on_signal, NULL);

    log_success (log_file, "Signals deferred or handled.");

}

static void
start_server (const char *conf_file) {
    int                 client_sd;
    int                 daemon_sd;
    int                 connected_sd;
    struct sockaddr_in  client_sa;
    struct sockaddr_in  daemon_sa;
    struct sockaddr_in  connected_sa;
    socklen_t           size;
    fd_set              sockets;
    int                 nfds;
    struct ifreq        if_info;
    struct sockaddr_in  *if_addr;

    prefs = conf_retrieve (conf_file);

    /* Create the shared directory if it does not exist already */
    if (create_dir (prefs->shared_folder, (mode_t)0755) < 0) {
        log_failure (log_file, "Unable to create shared directory");
        goto abort;
    }

    /* Initialize the list_lock semaphore */
    if (sem_init (&list_lock, 0, 1) < 0) {
        log_failure (log_file, "Unable to sem_init list_lock");
        goto abort;
    }
    list_client = NULL;

    /* Initialize the file_cache handler */
    if (sem_init (&file_cache_lock, 0, 1) < 0) {
        log_failure (log_file, "Unable to sem_init file_cache_lock");
        goto abort;
    }
    file_cache = NULL;

    /* Initializing the global semaphores */
    if (sem_init (&clients_lock, 0, 1) < 0) {
        log_failure (log_file, "Unable to sem_init clients_lock");
        goto abort;
    }
    if (sem_init (&daemons_lock, 0, 1) < 0) {
        log_failure (log_file, "Unable to sem_init daemons_lock");
        goto abort;
    }

    downloads = NULL;
    if (sem_init (&downloads_lock, 0, 1) < 0) {
        log_failure (log_file, "Unable to sem_init download_queue_lock");
        goto abort;
    }
    client_sa.sin_family        = AF_INET;
    client_sa.sin_addr.s_addr   = INADDR_ANY;
    client_sa.sin_port          = htons (prefs->client_port);
    client_sd = socket_init (&client_sa);
    if (client_sd < 0) {
        goto abort;
    }

    daemon_sa.sin_family        = AF_INET;
    daemon_sa.sin_addr.s_addr   = INADDR_ANY;
    daemon_sa.sin_port          = htons (prefs->daemon_port);
    daemon_sd = socket_init (&daemon_sa);
    if (daemon_sd < 0) {
        goto abort;
    }

#if 1
    /* We get our ip */
    memcpy (if_info.ifr_name, prefs->interface, strlen (prefs->interface) + 1);
    if (ioctl (daemon_sd, SIOCGIFADDR, &if_info) == -1) {
        log_failure (log_file, "Can't get my ip from interface");
        log_failure (log_file, "LOL ERRNO : %s\n", strerror (errno));
        goto abort;
    }
    if_addr = (struct sockaddr_in *)&if_info.ifr_addr;
    inet_ntop (AF_INET, &if_addr->sin_addr, my_ip, INET_ADDRSTRLEN);
    log_success (log_file, "Found my IP : %s", my_ip);
#endif
    /* sockets contains both client_sd and daemon_sd */
    FD_ZERO (&sockets);
    size = sizeof (connected_sa);
    nfds = NFDS (client_sd, daemon_sd);
    for (;;) {
        /*
         * It is VERY important to FD_SET at each loop, because select
         * will FD_UNSET the socket descriptors
         */
        FD_SET (client_sd, &sockets);
        FD_SET (daemon_sd, &sockets);

        /* Block until a socket is ready to accept */
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
            /* This should never happen : neither client nor daemon!? */
            log_failure (log_file, "Unknown connection");
        }
    }

abort:
    conf_free (prefs);
    exit (EXIT_FAILURE);
}

int
main (int argc, char *argv[])
{
    (void )argc;
    daemonize ();
    start_server (argv[1]);
    for (;;)
        sleep (1);
    conf_free (prefs);

    return EXIT_SUCCESS;
}

/*
 * Create a full path of directories
 */
static int create_dir (const char *path, mode_t mode) {
    char *sub_path;
    char *last_slash;

    if (!(sub_path = strdup (path)))
        return -1;
    last_slash = sub_path;

    /*
     * To create /a/b/c/d we must create /a, /a/b, /a/b/c and /a/b/c/d
     * To create a/b/c we must create a, a/b, a/b/c
     */
    while ((last_slash = strchr (last_slash + 1, '/'))) {
        *last_slash = '\0';
        if (mkdir (sub_path, mode) < 0) {
            if (errno != EEXIST) {
                goto out;
            }
        }
        *last_slash = '/';
    }
    /*
     * One last time to create the full path
     */
    if (mkdir (sub_path, mode) < 0) {
        if (errno != EEXIST)
            goto out;
    }
    free (sub_path);
    return 0;

out:
    free (sub_path);
    return -1;
}

