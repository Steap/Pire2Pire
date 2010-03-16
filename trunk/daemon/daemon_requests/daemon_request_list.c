#include <sys/stat.h>           // entry_stat

#include <stdio.h>              // NULL
#include <stdlib.h>             // malloc ()
#include <dirent.h>             // DIR

#include "../../util/md5/md5.h" // MDFile ()
#include "../../util/logger.h"  // log_failure ()
#include "../daemon.h"          // daemon_send ()
#include "../daemon_request.h"  // struct daemon_request

#define SHARED_FOLDER "/tmp/lol/"

extern FILE *log_file;
extern struct daemon *daemons;

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
daemon_request_list (void *arg) {
    struct daemon_request   *r;
    char                    answer[512];
    DIR                     *dir;
    struct dirent           *entry;
    char                    entry_full_path[256];
    struct stat             entry_stat;
    char                    *key;

    /* OKAY, let's say all options/args are silently ignored */

    r = (struct daemon_request *) arg;
    if (!r)
        return NULL;

    dir = opendir (SHARED_FOLDER);
    if (dir == NULL) {
        log_failure (log_file, "do_list () : can't open shared directory");
        return NULL;
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

            if (daemon_send (r->daemon, answer) < 0) {
                log_failure (log_file,
                            "do_list () : failed to send data to daemon");
            }

            free (key);
            key = NULL;
        }
    }

    if (closedir (dir) < 0) {
        log_failure (log_file, "do_list () : can't close shared directory");
        return NULL;
    }

    return NULL; 
}

