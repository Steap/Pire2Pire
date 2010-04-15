#include <arpa/inet.h>                              // inet_ntop ()
#include <netinet/in.h>                             // struct sockaddr_in

#include <pthread.h>                                // pthread_detach ()
#include <semaphore.h>                              // sem_t
#include <signal.h>                                 // struct sigaction
#include <stdlib.h>                                 // NULL
#include <string.h>                                 // strncmp ()
#include <unistd.h> //FIXME: remove when sleep () is removed

#include "../util/logger.h"                         // log_failure ()
#include "client.h"                                 // client_send ()
#include "client_request.h"                         // struct client_request
#include "client_requests.h"                    // client_request_connect ()
#include "util/socket.h"                            // socket_getline ()

#define MAX_CLIENTS                     2
#define MAX_REQUESTS_PER_CLIENT         2

extern FILE *log_file;

struct client *clients = NULL;
/*
 * This semaphore is meant to prevent multiple clients from modifying "clients"
 * all at the same time.
 *
 * It should be initialized by the very first client, and sort of never be
 * destroyed.
 */
sem_t          clients_lock;
#if 0
static void*
foo (void *a)
{
    struct request *r = (struct request *) a;
    struct request *tmp;
    char ans[100];

    sprintf (ans,
             " < Number of requests : %d\n",
             client_request_count (r->client->requests));
    send (r->client->socket, ans, strlen (ans), 0);
    for (tmp = r->client->requests; tmp; tmp = tmp->next) {
        sprintf (ans, "\t . %s\n", tmp->cmd);
        send (r->client->socket, ans, strlen (ans), 0);
    }
    sleep (1);

    sem_wait (&r->client->req_lock);
    r->client->requests = client_request_remove (r->client->requests, r->thread_id);
    sem_post (&r->client->req_lock);
    client_request_free (r);

    pthread_detach (pthread_self ());

    return NULL;
}
#endif

static void*
bar (void *a) {
    struct client_request   *r;
    r = (struct client_request *) a;
    client_send (r->client, "foo\n");
    sleep (5);
    log_failure (log_file, "I will now send bar");
    client_send (r->client, "bar\n");
    log_failure (log_file, "My client was : %s", r->client->addr);

    pthread_detach (pthread_self ());

    return NULL;
}



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
    struct client_request   *request;
};

static void *
start_request_thread (void *arg) {
    struct request_thread_wrapper   *wrapper;
    struct client_request           *r;
    struct sigaction                on_sigterm;

    /*
     * We must mask SIGTERM so that it won't call stop_server () but
     * terminate_thread () instead
     */
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
#if 1
    sem_wait (&r->client->req_lock);
    r->client->requests = client_request_remove (r->client->requests,
                                                    r->thread_id);
    sem_post (&r->client->req_lock);
    client_request_free (r);
#endif
    pthread_detach (pthread_self ());

    return NULL;
}



static void*
handle_requests (void *arg) {
    struct client                   *client;
    int                             r;
    /* The message typed by the user */
     char                            *message;
    void*                           (*callback) (void *);
    struct client_request           *request;
    struct request_thread_wrapper   wrapper;
    char                            answer[128];

    if (!(client = (struct client *) arg))
        goto out;

    log_success (log_file, "New client : %s", client->addr);

    for (;;)  {
        message  = NULL;
        callback = NULL;
        request  = NULL;

        message = socket_getline (client->socket);

        if (!message)
            goto out;

        /* Only request we're allowed to treat no matter how many requests are
         * currently being treated */
        if (strncmp (message, "quit", 4) == 0)
            goto out;

        if (client_request_count (client->requests) == MAX_REQUESTS_PER_CLIENT) {
            sprintf (answer, " < Too many requests, mister, plz calm down\n");
            send (client->socket, answer, strlen (answer), 0);
            continue;
        }

        /* Treating all the common requests */
        /* FIXME : use the IS_CMD macro */
        if (strncmp (message, "connect", 7) == 0)
            callback = &client_request_connect;
        else if (strncmp (message, "download", 8) == 0)
            callback = &client_request_download;
#if 1
        else if (strncmp (message, "get", 3) == 0)
            callback = &client_request_get;
#endif
        else if (strncmp (message, "help", 4) == 0)
            callback = &client_request_help;
        else if (strncmp (message, "info", 4) == 0)
            callback = &client_request_info;
        else if (strncmp (message, "list", 4) == 0)
            callback = &client_request_list;
        else if (strncmp (message, "raw", 3) == 0)
            callback = &client_request_raw;
        else if (strncmp (message, "set", 3) == 0)
            callback = &client_request_set;
        else if (strncmp (message, "bar", 3) == 0)
            callback = &bar;
        else
            callback = &client_request_unknown;
        request = client_request_new (message, client);

        if (!request) {
            sprintf (answer, " < Failed to create a new request\n");
            send (client->socket, answer, strlen (answer), 0);
            continue;
        }

        sem_wait (&client->req_lock);
        client->requests = client_request_add (client->requests, request);
        sem_post (&client->req_lock);

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
    log_success (log_file, "End of %s", client->addr);

    // Kill all client requests
    sem_wait (&client->req_lock);
    struct client_request *tmp;
    for (tmp = client->requests; tmp; tmp = tmp->next) {
        log_success (log_file, "Killing request %s", tmp->cmd);
        pthread_kill (tmp->thread_id, SIGTERM);
    }
    sem_post (&client->req_lock);

    // Then kill the client himself
    sem_wait (&clients_lock);
    clients = client_remove (clients, pthread_self ());
    sem_post (&clients_lock);

#if 1
    if (message) {
        free (message);
//        log_failure (log_file, "hr : Freed message");
    }
#endif
    if (request) {
//        log_failure (log_file, "hr : client_request_free ()");
        client_request_free (request);
    }
#if 1
    if (client) {
//        log_failure (log_file, "hr : client_free ()");
        client_free (client);
    }
#endif

    pthread_detach (pthread_self ());

    return NULL;
}


void
handle_client (int client_socket, struct sockaddr_in *client_addr) {
    int             r;
    struct client   *c;
    char            addr[INET_ADDRSTRLEN];

    c    = NULL;

    if (client_numbers (clients) == MAX_CLIENTS) {
        log_failure (log_file, "Too many clients");
        char answer[]= " < Too many clients right now\n";
        send (client_socket, answer, strlen (answer), 0);
        return;
    }

    if (inet_ntop (AF_INET,
                    &client_addr->sin_addr,
                    addr,
                    INET_ADDRSTRLEN)) {
        c = client_new (client_socket, addr);
    }

    if (!c)
        log_failure (log_file, "=/ :-( :'(");

    sem_wait (&clients_lock);
    clients = client_add (clients, c);
    sem_post (&clients_lock);

    if (!clients) {
        client_free (c);
        return;
    }

    //log_failure (log_file, "NUMBER %d", client_numbers (clients));

    r = pthread_create (&c->thread_id, NULL, handle_requests, c);
    if (r < 0) {
        log_failure (log_file, "==> Could not create thread");
        sem_wait (&clients_lock);
        clients = client_remove (clients, c->thread_id);
        sem_post (&clients_lock);
        return;
    }

}

