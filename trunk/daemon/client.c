#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

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
    sem_init (&c->socket_lock, 0, 1);

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
    log_failure (log_file, "about to remove a client");
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

int
client_send (struct client *c, const char *msg) {
    int     dest_sock;
    int     nb_sent;
    int     nb_sent_sum;
    int     send_size;
    char    ending_char;

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

    // This is to assure we send a full response
    if (msg[send_size - 1] != '\n') {
        ending_char = '\n';
        if (send (dest_sock, &ending_char, 1, 0) < 1) {
            log_failure (log_file,
                        "Failed to send a '\\n' terminated response");
            return -1;
        }
    }

    if (sem_post (&c->socket_lock) < 0) {
        log_failure (log_file, "Failed to sem_post socket_lock");
    }

    return 0;
}
