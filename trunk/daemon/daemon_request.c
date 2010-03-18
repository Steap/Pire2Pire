#include <pthread.h>        // pthread_equal ()
#include <stdlib.h>         // malloc ()

#include "../util/logger.h" // log_recv ()
#include "daemon_request.h" // struct daemon_request

struct daemon;

extern FILE*    log_file;

struct daemon_request *
daemon_request_new (char *cmd, struct daemon *daemon) {
    struct daemon_request *r;

    // Log the request (see ../util/logger.c to disactivate this)
    log_recv (log_file, cmd);

    r = malloc (sizeof (struct daemon_request));
    if (!r)
        return NULL;
    
//    r->cmd         = strdup (cmd);
    r->cmd          = cmd;
    r->daemon       = daemon;
    r->prev         = NULL;
    r->next         = NULL;
    r->thread_id    = -1;

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
daemon_request_remove (struct daemon_request *l, pthread_t pt) {
    if (!l)
        return NULL;
    
    struct daemon_request *tmp;
    for (tmp = l; pthread_equal (tmp->thread_id, pt) == 0; tmp = tmp->next);
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
