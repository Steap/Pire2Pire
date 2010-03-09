#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "client.h"

#include "../util/logger.h"

extern FILE *log_file;

struct client*
client_new (int socket, char *addr) {
    struct client *c;
    c = malloc (sizeof (struct client));
    if (c == NULL)
        return NULL;

   // c->id     = id;
    c->socket = socket;
    c->addr   = strdup (addr);
    c->next= NULL;
    c->prev =NULL;
    c->requests = NULL;
    sem_init (&c->req_lock, 0, 1);

    return c;
}

void 
client_free (struct client *c) {
    log_failure (log_file, "Freeing client %s", c->addr);
    if (!c)
        return;
    if (c->addr) {
        free (c->addr);
        c->addr = NULL;
    }
    sem_destroy (&c->req_lock);
    free (c);
}

struct client*
client_add (struct client *l, struct client *c) {
    if (!c) {
        return l;
    }
    if (!l) {
        return c;
    }
    c->prev = NULL;
    c->next = l;
    l->prev = c;
    return c;
}

int
client_numbers (struct client *l) {
    if (!l)
        return 0;
    struct client *tmp;
    int sum;
    for (tmp = l, sum = 0; tmp; tmp = tmp->next, sum++);
    return sum;
}

struct client*
client_remove (struct client *l, pthread_t pt) {
    log_failure (log_file, "about to remvoe a client");
    log_failure (log_file, "nb of clients : %d", client_numbers (l));
    if (!l)
        return NULL;
    struct client *tmp;
    for (tmp = l; pthread_equal (tmp->thread_id, pt) == 0; tmp = tmp->next);

    /* This was the last element */
    if (tmp->prev == NULL && tmp->next == NULL) {
        return NULL;
    }

    /* Was the first element */
    if (!tmp->prev) {
        tmp->next->prev = NULL;
        return tmp->next;
    }
    
    tmp->prev->next = tmp->next;
    return l;
}
