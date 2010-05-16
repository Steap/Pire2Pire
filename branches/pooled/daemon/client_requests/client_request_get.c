#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>

#include <arpa/inet.h>

#include <errno.h>
#include <fcntl.h>
#include <semaphore.h>
#include <stdio.h>              // FILE
#include <string.h>             // strlen ()
#include <unistd.h>

#include "../util/logger.h"  // log_failure ()
#include "../client.h"          // client_send ()
#include "../client_request.h"  // struct client_request
#include "../daemon.h"
#include "../util/cmd_parser.h" // struct option_template
#include "../util/socket.h"
#include "../file_cache.h"
#include "../conf.h"
#include "../dl_file.h"

#define RECV_BUFFSIZE   128

extern struct daemon        *daemons;
extern sem_t                daemons_lock;
extern FILE*                log_file;
extern struct file_cache    *file_cache;
extern sem_t                file_cache_lock;
extern struct prefs         *prefs;
extern sem_t                downloads_lock;
extern struct dl_file       *downloads;

extern struct prefs *prefs;

static const struct client_request  *r;
static char                         answer[256];
/* Will point the file cache entry of the file we will download */
static const struct file_cache      *file_to_dl;
/* Daemon who should send us the file */
static struct daemon                *d;

static int find_file ();
static int find_daemon ();

void*
client_request_get (void *arg) {
    r = (struct client_request *) arg;
    if (!r)
        return NULL;


    /* First we find the file in the cache */
    if (find_file () < 0)
        return NULL;
    /* file_to_dl should now be the good one */


    /* Find the daemon owning the file by checking its IP */
    if (find_daemon () < 0)
        return NULL;
    /* d should now point to the good daemon */


    /* Sending the "get key begin end" message */
    sprintf (answer,
             "get %s %d %ld\n",
             file_to_dl->key,
             0,
             file_to_dl->size);
    if (daemon_send (d, answer) < 0) {
        log_failure (log_file, "cr_get: could not send the get command");
        return NULL;
    }
/* FIXME */
#if 0
    sem_wait (&downloads_lock);
    char *full_path;
    struct dl_file *f;

    full_path = malloc (strlen (prefs->shared_folder)
                        + strlen (file_to_dl->filename) + 2);
    if (!full_path) {
        log_failure (log_file, "OMG NO MALLOC IS POSSIBLE");
        return NULL;
    }
    else {
        sprintf (full_path, "%s/%s",
                        prefs->shared_folder, file_to_dl->filename);
    }
    f = dl_file_new (full_path);
    if (!f) {
        log_failure (log_file, "SHARED FOLDER : %S", prefs->shared_folder);
        log_failure (log_file, "f was NULL %s", full_path);
    }
    downloads = dl_file_add (downloads, f);
    if (!downloads)
        log_failure (log_file, "downlaods was NULL");
    else
        log_failure (log_file, "downloads->path : %s", downloads->path);
    sem_post (&downloads_lock);
#endif

    return NULL;
}

static int find_file () {
    struct parsed_cmd   *parsed_get;

        if (cmd_parse_failed ((parsed_get = cmd_parse (r->cmd, NULL))))
            return -1;

        /*
         *  Checking whether the arguments are valid.  If they are, retrieves the
         *  data (stored in the cache) about the file to be downloaded
         */
        if (parsed_get->argc == 1) {
            if (client_send (r->client,
                                " < get: you need to specify a key\n") < 0)
                log_failure (log_file, "cr_get: client_send () failed");
            goto error;
        }
        else {
            if (strlen (parsed_get->argv[1]) != FILE_KEY_SIZE) {
                if (client_send (r->client,
                                    " < get: hash size is not good, man\n") < 0)
                    log_failure (log_file, "cr_get: client_send () failed");
                goto error;
            }
            else {
                sem_wait (&file_cache_lock);
                file_to_dl = file_cache_get_by_key (file_cache, parsed_get->argv[1]);
                sem_post (&file_cache_lock);
                if (!file_to_dl) {
                    if (client_send (r->client,
                                    " < get: key not in cache, please list\n") < 0)
                        log_failure (log_file, "cr_get: client_send () failed");
                    goto error;
                }
                sprintf (answer,
                        "< get : seeder is %s:%d\n",
                        file_to_dl->seeders->ip,
                        file_to_dl->seeders->port);
                if (client_send (r->client, answer) < 0) {
                    log_failure (log_file, "cr_get: client_send () failed");
                    goto error;
                }
            }
        }
        cmd_parse_free (parsed_get);

        if (client_send (r->client, answer) < 0) {
            log_failure (log_file, "cr_get: client_send () failed");
            return -1;
        }

        return 0;

    error:
        if (parsed_get)
            cmd_parse_free (parsed_get);
        return -1;
    }

    static int
    find_daemon () {
        char                    *addr;

        addr = file_to_dl->seeders->ip;
        sem_wait (&daemons_lock);
        /*
         * Retrieving the daemon
         * FIXME : use a function in daemon.c
         * Assigned to : roelandt
         */
        for_each_daemon (d) {
            if (strcmp (d->addr, addr) == 0)
                break;
        }
        sem_post (&daemons_lock);
        if (!d)
            return -1;

        return 0;
    }

