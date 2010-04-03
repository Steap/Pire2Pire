#include <arpa/inet.h>                              // inet_ntop ()
#include <netinet/in.h>                             // struct sockaddr_in

#include <pthread.h>                                // pthread_detach ()
#include <semaphore.h>                              // sem_t
#include <signal.h>                                 // struct sigaction
#include <stdlib.h>                                 // NULL
#include <string.h>                                 // strncmp ()

#include "../util/logger.h"                         // log_failure ()
#include "daemon.h"                                 // daemon_send ()
#include "daemon_request.h"                         // struct daemon_request
#include "daemon_requests.h"                    // daemon_request_unknown ()
#include "util/socket.h"                            // socket_getline ()

#define MAX_DAEMONS                     10
#define MAX_REQUESTS_PER_DAEMON         10

extern FILE *log_file;

struct daemon *daemons = NULL;
/*
 * This semaphore is meant to prevent multiple daemons from modifying "daemons"
 * all at the same time.
 *
 * It should be initialized by the very first daemon, and sort of never be
 * destroyed.
 */
sem_t          daemons_lock;



static void
terminate_thread (int signum) {
    (void)signum;
    pthread_detach (pthread_self ());
    pthread_exit (NULL);
}



/*
 * This wrapper is needed to send both the callback function and the
 * client_request via pthread_create ()
 */
struct request_thread_wrapper {
    void *                  (*callback) (void *);
    struct daemon_request   *request;
};

static void *
start_request_thread (void *arg) {
    struct request_thread_wrapper   *wrapper;
    struct daemon_request           *r;
    struct sigaction                on_sigterm;

    /*
     * We must mask SIGTERM so that it won't call stop_server () but
     * terminate_thread () instead
     */
    sigemptyset (&on_sigterm.sa_mask);
    on_sigterm.sa_handler = terminate_thread;
    on_sigterm.sa_flags = 0;
    sigemptyset (&on_sigterm.sa_mask);
    sigaction (SIGTERM, &on_sigterm, NULL);

    wrapper = (struct request_thread_wrapper *)arg;
    if (!wrapper)
        return NULL;
    r = wrapper->request;

    // Now we call the request handler
    wrapper->callback (wrapper->request);

    // And we remove the request properly
    sem_wait (&r->daemon->req_lock);
    r->daemon->requests = daemon_request_remove (r->daemon->requests,
                                                 r->thread_id);
    sem_post (&r->daemon->req_lock);
    daemon_request_free (r);
    pthread_detach (pthread_self ());

    return NULL;
}



static void*
handle_requests (void *arg) {
    struct daemon                   *daemon;
    int                             r;
    /* The message typed by the user */
    char                            *message;
    void*                           (*callback) (void *);
    struct daemon_request           *request;
    struct request_thread_wrapper   wrapper;
    char                            answer[128];

    if (!(daemon = (struct daemon *) arg))
        goto out;

    log_success (log_file, "New daemon : %s", daemon->addr);

    for (;;)  {
        message  = NULL;
        callback = NULL;
        request  = NULL;

        message = socket_getline (daemon->socket);

        if (!message)
            goto out;

        /* Only request we're allowed to treat no matter how many requests are
         * currently being treated */
        if (strncmp (message, "quit", 4) == 0)
            goto out;

        if (daemon_request_count (daemon->requests) == MAX_REQUESTS_PER_DAEMON) {
            sprintf (answer, " < Too many requests, mister, plz calm down\n");
            send (daemon->socket, answer, strlen (answer), 0);
            continue;
        }

        /* Treating all the common requests */
        /* FIXME : use the IS_CMD macro */
        if (strncmp (message, "list", 4) == 0)
            callback = &daemon_request_list;
        else if (strncmp (message, "get", 3) == 0)
            callback = &daemon_request_get;
        else if (strncmp (message, "file", 4) == 0)
            callback = &daemon_request_file;
        else if (strncmp (message, "neighbourhood", 13) == 0)
            callback = &daemon_request_neighbourhood;
        else if (strncmp (message, "neighbour", 9) == 0)
            callback = &daemon_request_neighbour;
        else if (strncmp (message, "ready", 5) == 0)
            callback = &daemon_request_ready;
        else
            callback = &daemon_request_unknown;

        request = daemon_request_new (message, daemon);

        if (!request) {
            sprintf (answer, " < Failed to create a new request\n");
            send (daemon->socket, answer, strlen (answer), 0);
            continue;
        }

        sem_wait (&daemon->req_lock);
        daemon->requests = daemon_request_add (daemon->requests, request);
        sem_post (&daemon->req_lock);

        wrapper.callback = callback;
        wrapper.request = request;
        r = pthread_create (&request->thread_id,
                            NULL,
                            start_request_thread,
                            &wrapper);

        if (r < 0) {
            sprintf (answer,
                    " < Could not treat your request for an unexpected \
                      reason.");
            log_failure (log_file, "Could not start new thread");
            continue;
        }
    }

out:
    log_success (log_file, "End of %s", daemon->addr);

    sem_wait (&daemons_lock);
    daemons = daemon_remove (daemons, pthread_self ());
    sem_post (&daemons_lock);

#if 1
    // Free : 1
    if (message) {
        free (message);
//        log_failure (log_file, "hr : Freed message");
    }
#endif
    if (request) {
        log_failure (log_file, "hr : request_free ()");
        daemon_request_free (request);
    }
#if 1
    // Free : 2
    if (daemon) {
//        log_failure (log_file, "hr : daemon_free ()");
        daemon_free (daemon);
    }
#endif

    pthread_detach (pthread_self ());

    return NULL;
}


void
handle_daemon (int daemon_socket, struct sockaddr_in *daemon_addr) {
    int             r;
    struct daemon   *d;
    char            addr[INET_ADDRSTRLEN];

    d    = NULL;

    if (daemon_numbers (daemons) == MAX_DAEMONS) {
        log_failure (log_file, "Too many daemons");
        char answer[]= " < Too many daemons right now\n";
        // FIXME: send must be whiled
        send (daemon_socket, answer, strlen (answer), 0);
        return;
    }

    if (inet_ntop (AF_INET,
                    &daemon_addr->sin_addr,
                    addr,
                    INET_ADDRSTRLEN)) {
        d = daemon_new (daemon_socket, addr, ntohs (daemon_addr->sin_port));
    }

    if (!d)
        log_failure (log_file, "=/ :-( :'(");

    sem_wait (&daemons_lock);
    daemons = daemon_add (daemons, d);
    sem_post (&daemons_lock);

    if (!daemons) {
        daemon_free (d);
        return;
    }

    r = pthread_create (&d->thread_id, NULL, handle_requests, d);
    if (r < 0) {
        log_failure (log_file, "==> Could not create thread");
        sem_wait (&daemons_lock);
        daemons = daemon_remove (daemons, d->thread_id);
        sem_post (&daemons_lock);
        return;
    }
}

