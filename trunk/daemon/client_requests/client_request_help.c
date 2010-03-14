#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>

#include "../client.h"
#include "../client_request.h"

#define HELP_TEXT   \
"\
====================================HELP====================================\n\
connect IP:PORT     Connects the daemon to another daemon                   \n\
get KEY             Has your daemon download the file whose key is KEY      \n\
help                Displays this message                                   \n\
info                Displays info about the daemon state                    \n\
list                Displays a list of available files on the network       \n\
set                 Lists available options                                 \n\
set option=value    Sets an option's value                                  \n\
quit                Quits                                                   \n\
============================================================================\n"

void*
client_request_help (void *arg) {
    struct client_request   *r;

    r = (struct client_request *) arg;
    if (!r)
        return NULL; 

    client_send (r->client, HELP_TEXT);

    sem_wait (&r->client->req_lock);
    r->client->requests = client_request_remove (r->client->requests, r->thread_id);
    sem_post (&r->client->req_lock);
    client_request_free (r);
    pthread_detach (pthread_self ());

    return NULL; 
}

