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
#include "../../util/md5/md5.h"

#define SHARED_FOLDER "/tmp/lol/"

extern FILE *log_file;
extern struct client *clients;

static char*
get_md5 (const char *path) {
    int           i;
    unsigned char digest[16];
    char          *hash;
    
    hash = malloc (33);
    MDFile (&digest, path);
    if (!hash)
        return NULL;
    hash[32] = '\0';
    for (i = 0; i < 16; i++) {
        sprintf (hash+2*i, "%02x", digest[i]);
    }
    return hash;
}

void*
client_request_list (void *arg) {
    struct client_request   *r;
    char                    answer[512];
    DIR                     *dir;
    struct dirent           *entry;
    char                    entry_full_path[256];
    struct stat             entry_stat;
    char                    *key;

    /* OKAY, let's say all options/args are silently ignored */

    r = (struct client_request *) arg;
    if (!r)
        return NULL;

    dir = opendir (SHARED_FOLDER);
    if (dir == NULL) {
        log_failure (log_file, "do_list () : can't open shared directory");
        goto out;
    }

    for (entry = readdir (dir); entry != NULL; entry = readdir (dir)) {
        if (entry->d_type == DT_REG) {
            sprintf (entry_full_path, "%s/%s", SHARED_FOLDER, entry->d_name);
            if (stat (entry_full_path, &entry_stat) < 0) {
                log_failure (log_file,
                            "do_list () : can't stat file %s",
                            entry_full_path);
                continue;
            }

            key = get_md5 (entry_full_path);
            if (!key)
                continue;

            sprintf (answer, "file %s %s %d\n",
                    entry->d_name,
                    key,
                    (int) entry_stat.st_size);

            if (client_send (r->client, answer) < 0) {
                log_failure (log_file,
                            "do_list () : failed to send data to client");
            }

            free (key);
            key = NULL;
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

