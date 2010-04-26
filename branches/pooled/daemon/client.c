#include <sys/socket.h>     // send ()

#include <stdlib.h>         // malloc ()
#include <string.h>         // strdup ()

#include "../util/logger.h" // log_failure ()
#include "client.h"         // struct client

extern FILE *log_file;

struct client*
client_new (int socket, char *addr) {
    struct client *c;
    c = malloc (sizeof (struct client));
    if (c == NULL)
        return NULL;

    c->socket = socket;
    c->addr   = strdup (addr);
    c->next= NULL;
    c->prev =NULL;
    c->requests = NULL;
    c->nb_requests = 0;
    sem_init (&c->req_lock, 0, 1);
    sem_init (&c->socket_lock, 0, 1);

    return c;
}

void
client_free (struct client *c) {
//    log_failure (log_file, "Freeing client %s", c->addr);
    if (!c)
        return;
    if (c->addr) {
        free (c->addr);
        c->addr = NULL;
    }
    sem_destroy (&c->req_lock);
    sem_destroy (&c->socket_lock);
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

struct client*
client_remove (struct client *l, struct client *c) {
    struct client *tmp;

    if (!l)
        return NULL;

    for (tmp = l; tmp != c; tmp = tmp->next);

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

int
client_send (struct client *c, const char *msg) {
    int     dest_sock;
    int     nb_sent;
    int     nb_sent_sum;
    int     send_size;

    dest_sock = c->socket;
    if (sem_wait (&c->socket_lock) < 0) {
        log_failure (log_file, "Failed to sem_wait socket_lock");
        return -1;
    }

    // Now the socket is locked for us, let's send!
    send_size = strlen (msg);

    // Send the command
    nb_sent_sum = 0;
    while (nb_sent_sum < send_size) {
        nb_sent = send (dest_sock,
                        msg + nb_sent_sum,
                        send_size - nb_sent_sum,
                        0);
        if (nb_sent < 0) {
            log_failure (log_file,
                        "Couldn't send to client socket");
            return -1;
        }
        nb_sent_sum += nb_sent;
    }

    if (sem_post (&c->socket_lock) < 0) {
        log_failure (log_file, "Failed to sem_post socket_lock");
    }

    return 0;
}
