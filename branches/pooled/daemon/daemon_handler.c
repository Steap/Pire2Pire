#include <arpa/inet.h>          // inet_ntop ()
#include <netinet/in.h>         // struct sockaddr_in

#include <pthread.h>            // pthread_detach ()
#include <semaphore.h>          // sem_t
#include <signal.h>             // struct sigaction
#include <stdlib.h>             // NULL
#include <string.h>             // strncmp ()
#include <unistd.h>             // close ()

#include "../util/logger.h"     // log_failure ()
#include "conf.h"
#include "daemon.h"             // daemon_send ()
#include "daemon_request.h"     // struct daemon_request
#include "daemon_requests.h"    // daemon_request_unknown ()
#include "shared_counter.h"
#include "thread_pool.h"
#include "util/socket.h"        // socket_getline ()
#include "util/cmd_parser.h"

extern FILE         *log_file;
extern struct prefs *prefs;

extern struct pool  *slow_pool;
extern struct pool  *fast_pool;

extern struct daemon            *daemons;
extern sem_t                    daemons_lock;
extern struct shared_counter    nb_daemons;

static void *
handle_requests (struct daemon *daemon) {
    /* The message typed by the user */
    char                            *message = NULL;
    struct daemon_request           *r;
    struct pool                     *pool;
    void*                           (*handler) (void *);

    log_success (log_file, "New daemon : %s", daemon->addr);

    for (;;)  {
        message = socket_getline (daemon->socket);
        if (!message)
            break;

        /* Only request we're allowed to treat no matter how many requests are
         * currently being treated */
        if (strncmp (message, "quit", 4) == 0)
            break;

        sem_wait (&daemon->req_lock);
        if (daemon->nb_requests == prefs->max_requests_per_daemon) {
            sem_post (&daemon->req_lock);
            /* FIXME: this is copy-paste from client, should be different */
            daemon_send (daemon,
                        " < Too many requests, mister, plz calm down\n");
            continue;
        }
        sem_post (&daemon->req_lock);

        /* Treating all the common requests */
        /* FIXME : use the IS_CMD macro */
        pool = fast_pool;
        if (strncmp (message, "list", 4) == 0) {
            pool = slow_pool;
            handler = daemon_request_list;
        }
        else if (strncmp (message, "get", 3) == 0)
            handler = daemon_request_get;
        else if (strncmp (message, "file", 4) == 0)
            handler = daemon_request_file;
        else if (strncmp (message, "neighbourhood", 13) == 0)
            handler = daemon_request_neighbourhood;
        else if (strncmp (message, "neighbour", 9) == 0)
            handler = daemon_request_neighbour;
        else if (strncmp (message, "ready", 5) == 0)
            handler = daemon_request_ready;
        else
            handler = daemon_request_unknown;

        r = daemon_request_new (message, daemon, pool, handler);
        if (!r) {
            daemon_send (daemon,  " < Failed to create a new request\n");
            continue;
        }

        sem_wait (&daemon->req_lock);
        daemon->requests = daemon_request_add (daemon->requests, r);
        if (!daemon->requests) {
            daemon_request_free (r);
            break;
        }
        sem_post (&daemon->req_lock);

        pool_queue (r->pool, daemon_request_handler, r);
    }

    if (message)
        free (message);

    return NULL;
}


void *
handle_daemon (void *arg) {
    struct daemon   *d;

    d    = (struct daemon *)arg;
    if (!d)
        goto out;

    sem_wait (&daemons_lock);
    daemons = daemon_add (daemons, d);
    sem_post (&daemons_lock);
    if (!daemons) {
        daemon_free (d);
        goto out;
    }

    log_success (log_file, "BEGIN   daemon %s", d->addr);
    handle_requests (d);
    log_success (log_file, "END     daemon %s", d->addr);

    sem_wait (&daemons_lock);
    daemons = daemon_remove (daemons, d);
    sem_post (&daemons_lock);
    daemon_free (d);

out:
    sem_wait (&nb_daemons.lock);
    --nb_daemons.count;
    sem_post (&nb_daemons.lock);

    return NULL;
}
