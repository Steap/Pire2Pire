#include <pthread.h>        // pthread_equal ()
#include <semaphore.h>
#include <stdlib.h>         // malloc ()

#include "../util/logger.h"
#include "client_request.h" // struct client_request

extern FILE *log_file;

struct client_request*
client_request_new (char *cmd,
                    struct client *client,
                    struct pool *pool,
                    void * (*func) (void *)) {
    struct client_request *r;
    r = malloc (sizeof (struct client_request));
    if (!r)
        return NULL;

//    r->cmd         = strdup (cmd);
    r->cmd          = cmd;
    r->client       = client;
    r->pool         = pool;
    r->handler      = func;
    r->prev         = NULL;
    r->next         = NULL;

    return r;
}

void
client_request_free (struct client_request *r) {
    if (!r)
        return;
    if (r->cmd) {
        free (r->cmd);
        r->cmd = NULL;
    }
    free (r);
}

struct client_request *
client_request_add (struct client_request *l, struct client_request *r) {
    if (!l)
        return r;
    if (!r)
        return l;
    r->prev = NULL;
    r->next = l;
    l->prev = r;
    return r;
}

struct client_request *
client_request_remove (struct client_request *l, struct client_request *r) {
    struct client_request *tmp;
log_failure (log_file, "Removing request %s", r->cmd);
    if (!l)
        return NULL;

    for (tmp = l; tmp != r; tmp = tmp->next);

    if (!tmp->prev) {
        if (tmp->next)
            tmp->next->prev = NULL;
        return tmp->next;
    }
    else {
        tmp->prev->next = tmp->next;
        if (tmp->next)
            tmp->next->prev = tmp->prev;
        return l;
    }
}

int
client_request_count (struct client_request *r) {
    struct client_request *tmp;
    int sum;
    for (tmp = r, sum = 0; tmp; tmp = tmp->next, sum++);
    return sum;
}

void * client_request_handler (void *arg) {
    struct client_request *r;
    /* TODO: cleanup_push ? */
    r = (struct client_request *)arg;

    sem_wait (&r->client->req_lock);
    ++r->client->nb_requests;
    sem_post (&r->client->req_lock);

    r->handler (r);

    sem_wait (&r->client->req_lock);
    r->client->requests = client_request_remove (r->client->requests, r);
    --r->client->nb_requests;
    sem_post (&r->client->req_lock);
    client_request_free (r);
    /* TODO: cleanup_pop ? */

    return NULL;
}
