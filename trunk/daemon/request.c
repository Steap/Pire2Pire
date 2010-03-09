#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "request.h"

struct client;

struct request*
request_new (char *cmd, struct client *client) {
    struct request *r;
    r = malloc (sizeof (struct request));
    if (!r)
        return NULL;
    
    r->cmd         = strdup (cmd);
    r->client      = client;
    r->prev        = NULL;
    r->next        = NULL;

    return r;
}

void
request_free (struct request *r) {
    if (!r)
        return;
    if (r->cmd) {
        free (r->cmd);
        r->cmd = NULL;
    }
    free (r);
}

struct request*
request_add (struct request *l, struct request *r) {
    if (!l)
        return r;
    if (!r)
        return l;
    r->prev = NULL;
    r->next = l;
    l->prev = r;
    return r;
}

struct request*
request_remove (struct request *l, pthread_t pt) {
    if (!l)
        return NULL;
    
    struct request *tmp;
    for (tmp = l; pthread_equal (tmp->thread_id, pt) == 0; tmp = tmp->next);
    if (!tmp->prev)
        return tmp->next;
    tmp->prev->next = tmp->next;
    return l;
}

int
request_count (struct request *r) {
    struct request *tmp;
    int sum;
    for (tmp = r, sum = 0; tmp; tmp = tmp->next, sum++);
    return sum;
}
