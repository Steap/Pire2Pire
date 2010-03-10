#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "../client.h"
#include "../request.h"
//#include "../callback_argument.h"

#include "../../util/logger.h"

struct client;

extern FILE *log_file;

void*
do_unknown_command (void* arg) {
#if 0
    char                      answer[256];
    struct callback_argument  *cba;
    
    cba = (struct callback_argument *) arg;
    if (!cba)
        return NULL;

    sprintf (answer, " < Unknown command '%s'\n", cba->cmd);
    if (send (cba->client_socket, answer, strlen (answer), 0) < 0) {
        log_failure (log_file, 
                     "do_unknown_command () : failed to send data back to the \
                     client");
        return NULL;
    }

    if (cba)
        cba_free (cba);
#endif

    struct request  *r;
    char             answer[256];
    
    r = (struct request *) arg;
    if (!r)
        return NULL;

    sprintf (answer, " < Unknown command '%s'\n", r->cmd);
    if (send (r->client->socket, answer, strlen (answer), 0) < 0) {
        log_failure (log_file, 
                     "do_unknown_command () : failed to send data back to the \
                     client");
        goto out;
    }
    
out:
    sem_wait (&r->client->req_lock);
    r->client->requests = request_remove (r->client->requests, r->thread_id);
    sem_post (&r->client->req_lock);
    request_free (r);
    pthread_detach (pthread_self ());

    return NULL;
}
