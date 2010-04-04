#include <arpa/inet.h>          // inet_ntop ()
#include <netinet/in.h>         // struct sockaddr_in

#include <pthread.h>            // pthread_detach ()
#include <semaphore.h>          // sem_t
#include <signal.h>             // struct sigaction
#include <stdlib.h>             // NULL
#include <string.h>             // strncmp ()
#include <unistd.h>             // close ()

#include "../util/logger.h"     // log_failure ()
#include "daemon.h"             // daemon_send ()
#include "daemon_request.h"     // struct daemon_request
#include "daemon_requests.h"    // daemon_request_unknown ()
#include "util/socket.h"        // socket_getline ()
#include "util/cmd_parser.h"

#define MAX_DAEMONS                     10
#define MAX_REQUESTS_PER_DAEMON         10
/* A daemon who connects has this number of  seconds to send a "neighbour"
 * message identifying itself */
#define IDENTIFICATION_TIMEOUT          10

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
    void *                          (*callback) (void *);
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

    /*
     * Unwrap what we need
     */
    wrapper = (struct request_thread_wrapper *)arg;
    if (!wrapper)
        return NULL;
    r = wrapper->request;
    callback = wrapper->callback;
    free (wrapper);

    sem_wait (&r->daemon->req_lock);
    r->daemon->requests = daemon_request_add (r->daemon->requests, r);
    sem_post (&r->daemon->req_lock);

    // Now we call the request handler
    callback (r);

    // And we remove the request properly
    sem_wait (&r->daemon->req_lock);
    r->daemon->requests = daemon_request_remove (r->daemon->requests,
                                                 r->thread_id);
    sem_post (&r->daemon->req_lock);

    daemon_request_free (r);
    pthread_detach (pthread_self ());

    return NULL;
}



void *
handle_requests (void *arg) {
    struct daemon                   *daemon;
    int                             r;
    /* The message typed by the user */
    char                            *message;
    void*                           (*callback) (void *);
    struct daemon_request           *request;
    struct request_thread_wrapper   *wrapper;
    char                            answer[128];

    if (!(daemon = (struct daemon *) arg))
        goto out;

    log_success (log_file, "New daemon : %s", daemon->addr);

    for (;;)  {
        wrapper = NULL;
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
            /* FIXME: this is copy-paste from client, should be different */
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

        wrapper = (struct request_thread_wrapper *)
                    malloc (sizeof (struct request_thread_wrapper));
        wrapper->callback = callback;
        wrapper->request = request;

        sem_wait (&daemon->req_lock);
        r = pthread_create (&request->thread_id,
                            NULL,
                            start_request_thread,
                            wrapper);
        sem_post (&daemon->req_lock);

        if (r) {
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

    if (message) {
        free (message);
    }
    if (request) {
        daemon_request_free (request);
    }
    if (daemon) {
        close (daemon->socket);
        daemon_free (daemon);
    }

    pthread_detach (pthread_self ());

    return NULL;
}


void
handle_daemon (int daemon_socket, struct sockaddr_in *daemon_addr) {
    int             r;
    struct daemon   *d;
    char            addr[INET_ADDRSTRLEN];
    char            *ident_msg;

    d    = NULL;

    if (daemon_numbers (daemons) == MAX_DAEMONS) {
        log_failure (log_file, "Too many daemons");
        char answer[]= " < Too many daemons right now\n";
        // FIXME: send must be whiled
        send (daemon_socket, answer, strlen (answer), 0);
        return;
    }

    if (!inet_ntop (AF_INET,
                    &daemon_addr->sin_addr,
                    addr,
                    INET_ADDRSTRLEN)) {
        return;
    }

    /*
     * The daemon who connected must identify itself or we won't add it
     */
    struct parsed_cmd   *pcmd;
    int                 port;
    char                *colon;
    char                answer[256];

    ident_msg = socket_try_getline (daemon_socket, IDENTIFICATION_TIMEOUT);
    if (!ident_msg) {
        sprintf (answer, "error: identification timed out\n");
        goto abort;
    }
    sprintf (answer, "error: identify with neighbour IP:PORT\n");
    if (cmd_parse_failed ((pcmd = cmd_parse (ident_msg, NULL)))) {
        pcmd = NULL;
        goto abort;
    }
    if (pcmd->argc < 2)
        goto abort;
    if (strcmp (pcmd->argv[0], "neighbour") != 0)
        goto abort;
    if (!(colon = strchr (pcmd->argv[1], ':')))
        goto abort;
    *colon = '\0';
    if (strcmp (pcmd->argv[1], addr) != 0) {
        sprintf (answer,
                "error: for me, you are (%s), not (%s)\n",
                addr,
                pcmd->argv[1]);
        goto abort;
    }
    port = atoi (colon + 1);

    free (ident_msg);
    cmd_parse_free (pcmd);

    d = daemon_new (daemon_socket, addr, port);
    if (!d)
        log_failure (log_file, "=/ :-( :'(");

    sem_wait (&daemons_lock);
    // FIXME: shouldn't we daemon_add after pthread_create?
    daemons = daemon_add (daemons, d);
    if (!daemons) {
        daemon_free (d);
        return;
    }
    sem_post (&daemons_lock);

    r = pthread_create (&d->thread_id, NULL, handle_requests, d);
    if (r < 0) {
        log_failure (log_file, "==> Could not create thread");
        sem_wait (&daemons_lock);
        // FIXME: is thread_id initialized here?
        daemons = daemon_remove (daemons, d->thread_id);
        sem_post (&daemons_lock);
        return;
    }

    return;

abort:
    socket_sendline (daemon_socket, answer);
    close (daemon_socket);
    if (ident_msg)
        free (ident_msg);
    return;
}

