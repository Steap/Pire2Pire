#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "client_request.h"

struct client;

struct client_request*
client_request_new (char *cmd, struct client *client) {
    struct client_request *r;
    r = malloc (sizeof (struct client_request));
    if (!r)
        return NULL;
    
//    r->cmd         = strdup (cmd);
    r->cmd = cmd;
    r->client      = client;
    r->prev        = NULL;
    r->next        = NULL;

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
client_request_remove (struct client_request *l, pthread_t pt) {
    if (!l)
        return NULL;
    
    struct client_request *tmp;
    for (tmp = l; pthread_equal (tmp->thread_id, pt) == 0; tmp = tmp->next);
    if (!tmp->prev)
        return tmp->next;
    tmp->prev->next = tmp->next;
    // FIXME: BUG WITH tmp->next->prev
    return l;
}

int
client_request_count (struct client_request *r) {
    struct client_request *tmp;
    int sum;
    for (tmp = r, sum = 0; tmp; tmp = tmp->next, sum++);
    return sum;
}
