#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "../client.h"
#include "../request.h"

#include "../../util/logger.h"

extern FILE *log_file;
extern struct client *clients;

void*
do_info (void *arg) {
    struct request   *r;
    struct request   *tmp;
    char             answer[256];
    int              socket;
    int              n;
    int              n_clients;

    /* OKAY, let's say all options/args are silently ignored */

    r = (struct request *) arg;
    if (!r)
        return NULL; 

    socket = r->client->socket;

    sem_wait (&r->client->req_lock);
    n_clients = client_numbers (clients);
    sprintf (answer, 
             " < INFO : there %s %d client%s connected.\n", 
             (n_clients > 1) ? "are":"is",
             n_clients,
             (n_clients > 1) ? "s":"");
    client_send (r->client, answer);

    sprintf (answer, 
             " < INFO : You have %d requests currently running.\n", 
             request_count (r->client->requests));
    client_send (r->client, answer);
   
    for (tmp = r->client->requests, n = 1; tmp; tmp = tmp->next, n++) {
        sprintf (answer,  " < INFO : %d - %s\n", n, tmp->cmd);
        client_send (r->client, answer);
    }
    sem_post (&r->client->req_lock);

//out:
    sem_wait (&r->client->req_lock);
    r->client->requests = request_remove (r->client->requests, r->thread_id);
    sem_post (&r->client->req_lock);
    request_free (r);
    pthread_detach (pthread_self ());

    return NULL; 
}

