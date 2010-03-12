#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "../client.h"
#include "../request.h"
//#include "../callback_argument.h"

#include "../../util/logger.h"

extern FILE *log_file;

void*
do_unknown_command (void* arg) {
    struct request  *r;
    char             answer[256];
    
    r = (struct request *) arg;
    if (!r)
        return NULL;

    sprintf (answer, " < Unknown command '%s'\n", r->cmd);
    if (client_send (r->client, answer) < 0) {
        log_failure (log_file, 
                     "do_unknown_command () : failed to send data back to the \
                     client");
        goto out;
    }
    
out:
    sem_wait (&r->client->req_lock);
    r->client->requests = request_remove (r->client->requests, r->thread_id);
    sem_post (&r->client->req_lock);
    log_failure (log_file, "do_() : request_free");
    request_free (r);
    log_failure (log_file, "REQUEST FREE IN UC");
    pthread_detach (pthread_self ());

    return NULL;
}
