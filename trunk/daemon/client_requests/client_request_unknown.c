#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../client.h"
#include "../client_request.h"
#include "../../util/logger.h"

extern FILE *log_file;

void*
client_request_unknown (void* arg) {
    struct client_request   *r;
    char                    answer[256];
    
    r = (struct client_request *) arg;
    if (!r)
        return NULL;

    sprintf (answer, "Unknown command '%s'\n", r->cmd);
    if (client_send (r->client, answer) < 0) {
        log_failure (log_file, 
                     "do_unknown_command () : failed to send data back to the \
                     client");
        goto out;
    }
    
out:
    sem_wait (&r->client->req_lock);
    r->client->requests = client_request_remove (r->client->requests, r->thread_id);
    sem_post (&r->client->req_lock);
    client_request_free (r);
    pthread_detach (pthread_self ());

    return NULL;
}
