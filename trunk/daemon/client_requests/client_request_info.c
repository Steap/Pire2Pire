#include <semaphore.h>          // sem_wait ()
#include <stdio.h>              // sprintf ()
#include <stdlib.h>             // NULL
#include <string.h>

#include "../client.h"          // struct client
#include "../client_request.h"  // struct client_request
#include "../daemon.h"          // daemon_numbers ()

#define DEBUG   1

extern struct client    *clients;
extern struct daemon    *daemons;
extern sem_t            daemons_lock;

void*
client_request_info (void *arg) {
    struct client_request   *r;
    struct client_request   *tmp;
    char                    answer[256];
    int                     n;
    int                     count;
    struct daemon           *d;

    r = (struct client_request *) arg;
    if (!r)
        return NULL;

    sem_wait (&r->client->req_lock);
    count = client_numbers (clients);
    sprintf (answer,
             " < INFO : there %s %d client%s connected.\n",
             (count > 1)?"are":"is",
             count,
             (count > 1)?"s":"");
    client_send (r->client, answer);

    count = daemon_numbers (daemons);
    sprintf (answer,
             " < INFO : there %s %d daemon%s connected.\n",
             (count > 1)?"are":"is",
             count,
             (count > 1)?"s":"");
    client_send (r->client, answer);

    count = client_request_count (r->client->requests);
    sprintf (answer,
             " < INFO : You have %d request%s currently running.\n",
             count,
             (count > 1)?"s":"");
    client_send (r->client, answer);

    for (tmp = r->client->requests, n = 1; tmp; tmp = tmp->next, n++) {
        sprintf (answer, " < INFO : %d - %s\n", n, tmp->cmd);
        client_send (r->client, answer);
    }
    sem_post (&r->client->req_lock);

#ifdef DEBUG
    client_send (r->client, "Daemons:\n");
    sem_wait (&daemons_lock);
    for_each_daemon (d) {
        sprintf (answer,
                "%s:%d\n",
                d->addr,
                d->port);
        client_send (r->client, answer);
    }
    sem_post (&daemons_lock);
#endif

    return NULL;
}

