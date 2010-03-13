#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../client.h"
#include "../daemon.h"
#include "../client_request.h"

#include "../../util/logger.h"

extern FILE *log_file;
extern struct client *clients;
extern struct daemon *daemons;

void*
do_info (void *arg) {
    struct client_request   *r;
    struct client_request   *tmp;
    char                    answer[256];
    int                     n;
    int                     count;

    /* OKAY, let's say all options/args are silently ignored */

    r = (struct client_request *) arg;
    if (!r)
        return NULL; 

    sem_wait (&r->client->req_lock);
    count = client_numbers (clients);
    sprintf (answer, 
             "INFO : there %s %d client%s connected.\n", 
             (count > 1)?"are":"is",
             count,
             (count > 1)?"s":"");
    client_send (r->client, answer);

    count = daemon_numbers (daemons);
    sprintf (answer, 
             "INFO : there %s %d daemon%s connected.\n", 
             (count > 1)?"are":"is",
             count,
             (count > 1)?"s":"");
    client_send (r->client, answer);

    count = client_request_count (r->client->requests);
    sprintf (answer, 
             "INFO : You have %d request%s currently running.\n", 
             count,
             (count > 1)?"s":"");
    client_send (r->client, answer);
   
    for (tmp = r->client->requests, n = 1; tmp; tmp = tmp->next, n++) {
        sprintf (answer, "INFO : %d - %s\n", n, tmp->cmd);
        client_send (r->client, answer);
    }
    sem_post (&r->client->req_lock);

//out:
    sem_wait (&r->client->req_lock);
    r->client->requests = client_request_remove (r->client->requests, r->thread_id);
    sem_post (&r->client->req_lock);
    client_request_free (r);
    pthread_detach (pthread_self ());

    return NULL; 
}

