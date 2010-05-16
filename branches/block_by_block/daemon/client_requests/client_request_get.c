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

#include "../../util/logger.h"  // log_failure ()
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

static int                  find_file ();
static int                  find_daemon ();
static struct daemon_list * find_daemons ();

void*
client_request_get (void *arg) {
    r = (struct client_request *) arg;
    struct daemon_list  *d_list;
    struct daemon_list  *list_item_to_delete;
    struct daemon       *next_d;
    int                 seeder_count;
    
    if (!r)
        return NULL;


    /* First we find the file in the cache */
    
    if (find_file () < 0)
        return NULL;
    /* file_to_dl should now be the good one */


    /* Find the daemons owning the file by checking its IP */
    if ((d_list = find_daemons ()) != NULL) {
        if (client_send (r->client,
            " < no seeder for the requested file. Former seeders must have been disconnected.\n") < 0)
            log_failure (log_file, "cr_get: client_send () failed");
        return NULL;
    }

    
    /* about handling multi-seeders and block-by-block download */
    
    // we know the number of seeders
    seeder_count = d_list->id;
    
    /* FIXME : calculate smartly the blocks to consider,
     * given :
     * - number of seeders
     * - size of file to dl
     * - a max_size for a block ? Not implemented
     */
     
    /* loop where we have to post a job for each seeder
     */
    next_d = d_list->daemon;
    while (next_d != NULL) {
        // FIXME : act !!!! do something !!!
        list_item_to_delete = d_list;
        d_list = d_list->next;
        free (list_item_to_delete);
    }



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



void*
internal_request_get (void *arg) {
    if(arg) {
        // used parameter.
    }
    /* FIXME : meant to be called for partial download, 
     * internally determined by client_request_get.
     */
     
     
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
    
/*
 * rather than finding one daemon only, we store every seeder daemons.
 *
 */
static struct daemon_list *
find_daemons () {
        int                     seed_count;
        struct seeder           *next_seeder;
        struct daemon_list      *d_list;
        struct daemon_list      *new_daemon;
        
        next_seeder = file_to_dl->seeders;
        seed_count = 0;
        d_list = NULL;
        while (next_seeder != NULL) {
            seed_count ++;
            for_each_daemon(d) {
                if (strcmp (d->addr, next_seeder->ip) == 0) {
                    new_daemon = (struct daemon_list *)malloc(sizeof(struct daemon_list));
                    new_daemon->daemon = d;
                    new_daemon->id = seed_count;
                    new_daemon->next = d_list;
                    d_list = new_daemon;
                    break;
                }         
            }
            next_seeder = next_seeder->next;
            
        }

        return d_list;
}


