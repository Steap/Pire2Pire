#include <arpa/inet.h>                              // inet_ntop ()
#include <netinet/in.h>                             // struct sockaddr_in

#include <pthread.h>                                // pthread_detach ()
#include <semaphore.h>                              // sem_t
#include <signal.h>                                 // struct sigaction
#include <stdlib.h>                                 // NULL
#include <string.h>                                 // strncmp ()
#include <unistd.h> //FIXME: remove when sleep () is removed

#include "../util/logger.h"                         // log_failure ()
#include "is_cmd.h"
#include "client.h"                                 // client_send ()
#include "client_request.h"                         // struct client_request
#include "client_requests.h"                    // client_request_connect ()
#include "conf.h"
#include "shared_counter.h"
#include "thread_pool.h"
#include "util/socket.h"                            // socket_getline ()

extern FILE         *log_file;
extern struct prefs *prefs;

extern struct pool  *slow_pool;
extern struct pool  *fast_pool;

extern struct client            *clients;
extern sem_t                    clients_lock;
extern struct shared_counter    nb_clients;

#if 0
/*
 * Useless function that is sometimes used to test stuff.
 */
static void*
bar (void *a) {
    struct client_request   *r;
    r = (struct client_request *) a;
    client_send (r->client, "foo\n");
    sleep (5);
    log_failure (log_file, "I will now send bar");
    client_send (r->client, "bar\n");
    log_failure (log_file, "My client was : %s", r->client->addr);

    return NULL;
}
#endif

static void*
handle_requests (struct client *client) {
    /* The message typed by the user */
    char                    *message = NULL;
    struct client_request   *r = NULL;
    struct pool             *pool;
    void *                  (*handler) (void *);

    for (;;)  {
        message = socket_getline (client->socket);
        if (!message)
            break;

        /* Only request we're allowed to treat no matter how many requests are
         * currently being treated */
        if (strncmp (message, "quit", 4) == 0)
            break;

        sem_wait (&client->req_lock);
        if (client->nb_requests == prefs->max_requests_per_client) {
            sem_post (&client->req_lock);
            client_send (client,
                        " < Too many requests, mister, plz calm down\n");
            continue;
        }
        sem_post (&client->req_lock);
        /* Treating all the common requests */
        /* FIXME : use the IS_CMD macro */
        pool = fast_pool;
#if 0
        if (strncmp (message, "connect", 7) == 0) {
            /* If the daemon does not exist, it takes time to timeout */
            pool = slow_pool;
            handler = client_request_connect;
        }
        else if (strncmp (message, "download", 8) == 0)
            handler = client_request_download;
        else if (strncmp (message, "get", 3) == 0)
            handler = client_request_get;
//        else if (strncmp (message, "help", 4) == 0)
        if (IS_CMD (message, "help"))
            handler = client_request_help;
        else if (strncmp (message, "info", 4) == 0)
            handler = client_request_info;
        else if (strncmp (message, "list", 4) == 0)
            handler = client_request_list;
        else if (strncmp (message, "raw", 3) == 0)
            handler = client_request_raw;
        else if (strncmp (message, "set", 3) == 0)
            handler = client_request_set;
        else if (strncmp (message, "bar", 3) == 0) {
            pool = slow_pool;
            handler = bar;
        }
#endif
        if (IS_CMD (message, "connect")) {
            pool = slow_pool;
            handler = client_request_connect;
        }
        else if (IS_CMD (message, "download"))
            handler = &client_request_download;
        else if (IS_CMD (message, "get"))
            handler = &client_request_get;
        else if (IS_CMD (message, "help"))
            handler = &client_request_help;
        else if (IS_CMD (message, "info"))
            handler = &client_request_info;
        else if (IS_CMD (message, "list"))
            handler = &client_request_list;
        else if (IS_CMD (message, "raw"))
            handler = &client_request_raw;
        else if (IS_CMD (message, "set"))
            handler = &client_request_set;
        else
            handler = &client_request_unknown;

        r = client_request_new (message, client, pool, handler);
        if (!r) {
            client_send (client, " < Failed to create a new request\n");
            continue;
        }

        sem_wait (&client->req_lock);
        client->requests = client_request_add (client->requests, r);
        if (!client->requests) {
            client_request_free (r);
            break;
        }
        sem_post (&client->req_lock);

        pool_queue (r->pool, client_request_handler, r);
    }

    if (message)
        free (message);

    return NULL;
}


void *
handle_client (void *arg) {
    struct client           *c;
    struct client_request   *tmp;
    struct client_request   *next;

    c = (struct client *)arg;
    if (!c)
        return NULL;

    sem_wait (&clients_lock);
    clients = client_add (clients, c);
    sem_post (&clients_lock);
    if (!clients) {
        client_free (c);
        return NULL;
    }

    log_success (log_file, "BEGIN   client %s", c->addr);
    handle_requests (c);
    log_success (log_file, "END     client %s", c->addr);

    sem_wait (&clients_lock);
    clients = client_remove (clients, c);
    sem_post (&clients_lock);

    /* Let's clean all remaining requests for this client */
    sem_wait (&c->req_lock);
    for (tmp = c->requests; tmp; tmp = next) {
        /* Either it's been assigned to a thread in the pool */
        if (tmp->assigned)
            pool_kill (tmp->pool, tmp->tid);
        /* Or it's still in the queue */
        else
            pool_flush_by_arg (tmp->pool, tmp);
        next = tmp->next;
        client_request_free (tmp);
    }
    sem_post (&c->req_lock);

    client_free (c);

    return NULL;
}

