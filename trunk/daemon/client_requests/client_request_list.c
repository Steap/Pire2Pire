#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>

#include "../client.h"
#include "../client_request.h"

#include "../../util/logger.h"

extern FILE *log_file;
extern struct client *clients;

void*
client_request_list (void *arg) {
    struct client_request   *r;
    char                    answer[512];
    DIR                     *dir;
    struct dirent           *entry;
    char                    entry_full_path[256];
    struct stat             entry_stat;

    /* OKAY, let's say all options/args are silently ignored */

    r = (struct client_request *) arg;
    if (!r)
        return NULL;

    dir = opendir ("/tmp/");
    if (dir == NULL) {
        log_failure (log_file, "do_list () : can't open shared directory");
        goto out;
    }

    for (entry = readdir (dir); entry != NULL; entry = readdir (dir)) {
        // Is it a regular file?
        if (entry->d_type == DT_REG) {
            // Build the file name
            sprintf (entry_full_path, "/tmp/%s", entry->d_name);
            // Retrieve its data
            if (stat (entry_full_path, &entry_stat) < 0) {
                log_failure (log_file,
                            "do_list () : can't stat file %s",
                            entry_full_path);
                continue;
            }
            sprintf (answer, "file %s [key] %d\n",
                    entry->d_name,
                    (int)entry_stat.st_size);
            if (client_send (r->client, answer) < 0) {
                log_failure (log_file,
                            "do_list () : failed to send data to client");
            }
        }
    }

    if (closedir (dir) < 0) {
        log_failure (log_file, "do_list () : can't close shared directory");
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

