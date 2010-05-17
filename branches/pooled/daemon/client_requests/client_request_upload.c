#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "../util/cmd.h"
#include "../client.h"
#include "../client_request.h"
#include "../dl_file.h"

#include "../util/logger.h"

extern FILE              *log_file;
extern struct dl_file    *uploads;
extern sem_t             uploads_lock;


/*
 * We silently ignore options/arguments, according to the protocol.
 */
void *
client_request_upload (void *arg) {
    struct client_request    *r;
    struct dl_file           *tmp;
    struct stat              stat_buf;
    char answer[512];
   
    r = (struct client_request *) arg; 
    if (!r)
        return NULL;

    sem_wait (&uploads_lock);
    if (!uploads) {
        sprintf (answer, 
                 "No files are currently being uploaded.\n");
        if (client_send (r->client, answer) < 0) {
            log_failure (log_file,
                        "client_request_upload () : client_send () failed");
        }
        sem_post (&uploads_lock);
        goto out;
    }
    
    for (tmp = uploads; tmp; tmp = tmp->next) {
        stat (tmp->path, &stat_buf);
        sprintf (answer, 
                "Uploading file %s\n",
                tmp->path);
        if (client_send (r->client, answer) < 0) {
            log_failure (log_file,
                        "client_request_upload () : client_send () failed");
        }
    }
    sem_post (&uploads_lock);

out:
    return NULL;
}
