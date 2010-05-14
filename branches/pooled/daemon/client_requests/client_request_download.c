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

#include "../../util/logger.h"

extern FILE              *log_file;
extern struct dl_file    *downloads;
extern sem_t             downloads_lock;


/*
 * We silently ignore options/arguments, according to the protocol.
 */
void *
client_request_download (void *arg) {
    struct client_request    *r;
    struct dl_file           *tmp;
    struct stat              stat_buf;
    char answer[512];
   
    r = (struct client_request *) arg; 
    if (!r)
        return NULL;

    sem_wait (&downloads_lock);
    if (!downloads) {
        sprintf (answer, 
                 "No files are currently being downloaded.\n");
        if (client_send (r->client, answer) < 0) {
            log_failure (log_file,
                        "client_request_download () : client_send () failed");
        }
        sem_post (&downloads_lock);
        goto out;
    }
    
    for (tmp = downloads; tmp; tmp = tmp->next) {
        stat (tmp->path, &stat_buf);
        sprintf (answer, 
                "Downloading file %s %.2f %c %d %d \n", 
                tmp->path,
                (float) stat_buf.st_size/ (float)tmp->total_size * 100,        
               '%',
                (int) stat_buf.st_size,
                (int) tmp->total_size);
        if (client_send (r->client, answer) < 0) {
            log_failure (log_file,
                        "client_request_download () : client_send () failed");
        }
    }
    sem_post (&downloads_lock);

out:
    return NULL;
}
