#include <stdlib.h>         // malloc ()

#include "../util/logger.h" // log_recv ()
#include "daemon_request.h" // struct daemon_request

extern FILE*    log_file;

struct daemon_request *
daemon_request_new (char *cmd,
                    struct daemon *daemon,
                    struct pool *pool,
                    void * (*func) (void *)) {
    struct daemon_request *r;

    // Log the request (see ../util/logger.c to disactivate this)
    log_recv (log_file, cmd);

    r = malloc (sizeof (struct daemon_request));
    if (!r)
        return NULL;

    r->cmd          = cmd;
    r->daemon       = daemon;
    r->pool         = pool;
    r->handler      = func;
    /* There is no portable way of initializing this field, so let's put a
        dummy pthread_t value in it for now, and an "assigned" boolean to know
        if the job is already assigned */
    r->tid          = pthread_self ();
    r->assigned     = 0;
    r->prev         = NULL;
    r->next         = NULL;

    return r;
}

void
daemon_request_free (struct daemon_request *r) {
    if (!r)
        return;
    if (r->cmd) {
        free (r->cmd);
        r->cmd = NULL;
    }
    free (r);
}

struct daemon_request *
daemon_request_add (struct daemon_request *l, struct daemon_request *r) {
    if (!l)
        return r;
    if (!r)
        return l;
    r->prev = NULL;
    r->next = l;
    l->prev = r;

    return r;
}

struct daemon_request*
daemon_request_remove (struct daemon_request *l, struct daemon_request *r) {
    struct daemon_request *tmp;

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
daemon_request_count (struct daemon_request *r) {
    struct daemon_request *tmp;
    int sum;
    for (tmp = r, sum = 0; tmp; tmp = tmp->next, sum++);
    return sum;
}

void *daemon_request_handler (void *arg) {
    struct daemon_request *r;
    /* TODO: cleanup_push ? */
    r = (struct daemon_request *)arg;

    sem_wait (&r->daemon->req_lock);
    ++r->daemon->nb_requests;
    r->tid = pthread_self ();
    r->assigned = 1;
    sem_post (&r->daemon->req_lock);

    r->handler (r);

    sem_wait (&r->daemon->req_lock);
    r->daemon->requests = daemon_request_remove (r->daemon->requests, r);
    --r->daemon->nb_requests;
    sem_post (&r->daemon->req_lock);
    daemon_request_free (r);
    /* TODO: cleanup_pop ? */

    return NULL;

}
