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
#include "client_request.h"
#include "daemon_request.h"
#include "conf.h"           // struct prefs
#include "daemon_handler.h" // handle_client ()
#include "dl_file.h"
#include "file_cache.h"     // struct file_cache
#include "shared_counter.h"
#include "thread_pool.h"
#include "util/cmd_parser.h"
#include "util/socket.h"    // socket_init ()

// nfds must be the maximum sd + 1
#define NFDS(a,b)   (((a)>(b))?(a+1):(b+1))

#define ABORT_IF(cmd, log_msg) if (cmd) {   \
    log_failure (log_file, log_msg);        \
    goto abort;                             \
}

#define IDENTIFICATION_TIMEOUT          10

FILE *log_file;
char my_ip[INET_ADDRSTRLEN];

/* Preferences defined in daemon.conf. */
struct prefs *prefs;

/* Pools of threads */
struct pool             *slow_pool;
struct pool             *fast_pool;
struct pool             *clients_pool;
struct shared_counter   nb_clients;
struct pool             *daemons_pool;
struct shared_counter   nb_daemons;

/* Connected clients */
sem_t            clients_lock;
struct client    *clients;
/* Connected daemons */
sem_t            daemons_lock;
struct daemon    *daemons;
/* Who is doing list? */
sem_t           list_lock;
struct client   *list_client;
/* TODO: comment plz */
sem_t                   downloads_lock;
struct dl_file          *downloads;

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
start_logger (void) {
    log_file = fopen (prefs->log_file, "w");
    
    if (!log_file) {
        fprintf (stderr, 
"Could not open the log file. There will be no logs.");
    }
    
}

static void
server_stop (int sig) {
    struct client   *c;
    struct daemon   *d;

    pool_destroy (slow_pool);
    pool_destroy (fast_pool);
    pool_destroy (clients_pool);
    pool_destroy (daemons_pool);

    (void) sig;
    log_failure (log_file, "Ok, received a signal");
    sleep (2);

    sem_destroy (&clients_lock);
    sem_destroy (&daemons_lock);
    sem_destroy (&file_cache_lock);
    sem_destroy (&downloads_lock);

    if (clients) {
        while (clients) {
            c = clients->next;
            client_free (clients);
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

    if (unlink (prefs->lock_file) < 0)
        if (log_file)
            log_failure (log_file, "Could not destroy the lock file");
    if (prefs) {
        conf_free (prefs);
        log_success (log_file,
                     "%s : Preferences have been freed.",
                     __func__);
    }

    if (log_file) {
        log_success (log_file, "Stopping server, waiting for SIGKILL");
        fclose (log_file);
    }

    exit (EXIT_SUCCESS);
}

void daemonize (void) {
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

    setsid ();
    log_success (log_file, "setsid ok");

    close (STDIN_FILENO);
    close (STDOUT_FILENO);
    close (STDERR_FILENO);
    log_success (log_file, "Closed stdin, stdout, stderr.");

    umask (027);
    log_success (log_file, "Set file permissions to 750.");

    lock = open (prefs->lock_file, O_RDWR | O_CREAT, 0640);
    if (lock < 0) {
        log_failure (log_file, 
                     "Failed to open lock file (%s).", 
                     prefs->lock_file);
    }
    else
        log_success (log_file, "Opened lock file (%s).", prefs->lock_file);

    if (lockf (lock, F_TLOCK, 0) < 0) {
        log_failure (log_file, "Could not lock %s", prefs->lock_file);
    }

    sprintf (str, "%d\n", getpid ());
    write (lock, str, strlen (str));
    if (close (lock) < 0) {
        log_failure (log_file, 
                     "Failed to close LOCK_FILE (%s).", 
                    prefs->lock_file);
    }
    else
        log_success (log_file, "Created lock file (%s)", prefs->lock_file);

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
start_server (void) {
    int                 client_sd;
    int                 daemon_sd;
    int                 connected_sd;
    struct sockaddr_in  client_sa;
    struct sockaddr_in  daemon_sa;
    struct sockaddr_in  connected_sa;
    socklen_t           size;
    fd_set              socket_set;
    int                 nfds;
    struct ifreq        if_info;
    struct sockaddr_in  *if_addr;
    char                addr[INET_ADDRSTRLEN];
    struct client       *c;
    struct daemon       *d;
    struct parsed_cmd   *pcmd = NULL;
    char                *ident_msg;
    int                 port;
    char                *colon;


    /* Prepare all the threads */
    slow_pool = NULL;
    fast_pool = NULL;
    clients_pool = NULL;
    daemons_pool = NULL;

    ABORT_IF (!(slow_pool = pool_create (prefs->nb_proc)),
        "Unable to create slow_pool")

    ABORT_IF (!(fast_pool = pool_create (prefs->nb_proc)),
        "Unable to create fast_pool")

    ABORT_IF (!(clients_pool = pool_create (prefs->max_clients)),
        "Unable to create clients_pool")

    ABORT_IF (!(daemons_pool = pool_create (prefs->max_daemons)),
        "Unable to create daemons_pool")

    nb_clients.count = 0;
    ABORT_IF (sem_init (&nb_clients.lock, 0, 1) < 0,
        "Unable to create nb_clients lock")

    nb_daemons.count = 0;
    ABORT_IF (sem_init (&nb_daemons.lock, 0, 1) < 0,
        "Unable to create nb_daemons lock")

    /* Create the shared directory if it does not exist already */
    ABORT_IF (create_dir (prefs->shared_folder, (mode_t)0755) < 0,
                "Unable to create shared directory")

    /* Initialize global pointers and their semaphores */
    clients = NULL;
    ABORT_IF (sem_init (&clients_lock, 0, 1) < 0,
        "Unable to sem_init clients_lock")

    daemons = NULL;
    ABORT_IF (sem_init (&daemons_lock, 0, 1) < 0,
        "Unable to sem_init daemons_lock")

    file_cache = NULL;
    ABORT_IF (sem_init (&file_cache_lock, 0, 1) < 0,
        "Unable to sem_init file_cache_lock")

    list_client = NULL;
    ABORT_IF (sem_init (&list_lock, 0, 1) < 0,
        "Unable to sem_init list_lock")

    downloads = NULL;
    ABORT_IF (sem_init (&downloads_lock, 0, 1) < 0,
        "Unable to sem_init download_queue_lock")

    client_sa.sin_family        = AF_INET;
    client_sa.sin_addr.s_addr   = INADDR_ANY;
    client_sa.sin_port          = htons (prefs->client_port);
    client_sd = socket_init (&client_sa);
    ABORT_IF (client_sd < 0,
        "Unable to socket_init client_sd")

    daemon_sa.sin_family        = AF_INET;
    daemon_sa.sin_addr.s_addr   = INADDR_ANY;
    daemon_sa.sin_port          = htons (prefs->daemon_port);
    daemon_sd = socket_init (&daemon_sa);
    ABORT_IF (daemon_sd < 0,
        "Unable to socket_init daemon_sd")

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

    /* socket_set contains both client_sd and daemon_sd */
    FD_ZERO (&socket_set);
    size = sizeof (connected_sa);
    nfds = NFDS (client_sd, daemon_sd);
    for (;;) {
        /*
         * It is VERY important to FD_SET at each loop, because select
         * will FD_UNSET the socket descriptors
         */
        FD_SET (client_sd, &socket_set);
        FD_SET (daemon_sd, &socket_set);

        /* Block until a socket is ready to accept */
        if (select (nfds, &socket_set, NULL, NULL, NULL) < 0) {
            log_failure (log_file, "main () : select failed");
        }

        if (FD_ISSET (client_sd, &socket_set)) {
            if ((connected_sd = (accept (client_sd,
                                        (struct sockaddr *) &connected_sa,
                                        &size))) < 0) {
                log_failure (log_file, "Failed to accept incoming connection.");
                break;
            }

            /* Can we handle this client? */
            sem_wait (&nb_clients.lock);
            if (nb_clients.count < prefs->max_clients) {
                nb_clients.count++;
                sem_post (&nb_clients.lock);
            }
            else {
                sem_post (&nb_clients.lock);
                socket_sendline (connected_sd, " < Too many clients\n");
                goto close_socket;
            }

            /* Then, let's handle him */
            if (!inet_ntop (AF_INET, &connected_sa.sin_addr,
                            addr, INET_ADDRSTRLEN)) {
                socket_sendline (connected_sd, " < Oops\n");
                goto failed_handling_client;
            }
            if (!(c = client_new (connected_sd, addr))) {
                socket_sendline (connected_sd, " < Sorry pal :(\n");
            }

            pool_queue (clients_pool, handle_client, c);
        }
        else if (FD_ISSET (daemon_sd, &socket_set)) {
            if ((connected_sd = (accept (daemon_sd,
                                        (struct sockaddr *) &connected_sa,
                                        &size))) < 0) {
                log_failure (log_file, "Failed to accept incoming connection.");
                break;
            }

            /* Can we handle this daemon? */
            sem_wait (&nb_daemons.lock);
            if (nb_daemons.count < prefs->max_daemons) {
                /* Remember client_request_connect is also creating daemons, so
                    we increment the counter even if we have to decrement it
                    on a possible following failure */
                nb_daemons.count++;
                sem_post (&nb_daemons.lock);
            }
            else {
                sem_post (&nb_daemons.lock);
                socket_sendline (connected_sd, " < Too many daemons\n");
                goto close_socket;
            }

            /* Let's identify him first */
            ident_msg = socket_try_getline (connected_sd,
                                            IDENTIFICATION_TIMEOUT);
            if (!ident_msg) {
                socket_sendline (connected_sd,
                                "error: identification timed out\n");
                goto failed_handling_daemon;
            }
            if (cmd_parse_failed ((pcmd = cmd_parse (ident_msg, NULL)))) {
                pcmd = NULL;
                goto failed_handling_daemon;
            }
            if (pcmd->argc < 2)
                goto failed_handling_daemon;
            if (strcmp (pcmd->argv[0], "neighbour") != 0)
                goto failed_handling_daemon;
            if (!(colon = strchr (pcmd->argv[1], ':')))
                goto failed_handling_daemon;
            port = atoi (colon + 1);

            free (ident_msg);
            cmd_parse_free (pcmd);
            pcmd = NULL;

            if (!inet_ntop (AF_INET, &connected_sa.sin_addr,
                            addr, INET_ADDRSTRLEN)) {
                socket_sendline (connected_sd, " < Oops\n");
                goto failed_handling_daemon;
            }

            /* Now we've got his port, let him go in */
            if (!(d = daemon_new (connected_sd, addr, port))) {
                socket_sendline (connected_sd, " < Sorry pal :(\n");
                goto failed_handling_daemon;
            }

            pool_queue (daemons_pool, handle_daemon, d);
        }
        else {
            /* This should never happen : neither client nor daemon!? */
            log_failure (log_file, "Unknown connection");
        }

        continue;

failed_handling_client:
        sem_wait (&nb_clients.lock);
        --nb_clients.count;
        sem_post (&nb_clients.lock);
        goto close_socket;

/* We jump here if we reserved a slot for a daemon, but failed to create it */
failed_handling_daemon:
        sem_wait (&nb_daemons.lock);
        --nb_daemons.count;
        sem_post (&nb_daemons.lock);
        /* goto close_socket; */

close_socket:
        if (pcmd) {
            cmd_parse_free (pcmd);
            pcmd = NULL;
        }
        close (connected_sd);
    }

abort:
    if (slow_pool)
        pool_destroy (slow_pool);
    if (fast_pool)
        pool_destroy (fast_pool);
    if (clients_pool)
        pool_destroy (clients_pool);
    if (daemons_pool)
        pool_destroy (daemons_pool);
    conf_free (prefs);
    exit (EXIT_FAILURE);
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

int
main (int argc, char *argv[])
{
    (void) argc;

    prefs = conf_retrieve (argv[1]);
    if (!prefs) {
        fprintf (stderr,
"Unable to retrieve preferences. Default preferences cannot be used. Aborting.\n");
    }
    
    start_logger ();
    daemonize ();
    start_server ();

    return EXIT_SUCCESS;
}

