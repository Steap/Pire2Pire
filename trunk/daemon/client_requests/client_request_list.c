#include <sys/stat.h>           // entry_stat

#include <stdio.h>              // NULL
#include <stdlib.h>             // malloc ()
#include <dirent.h>             // DIR

#include "../../util/md5/md5.h" // MDFile ()
#include "../../util/logger.h"  // log_failure ()
#include "../client.h"          // client_send ()
#include "../client_request.h"  // struct client_request

#define SHARED_FOLDER "/tmp/lol/"

extern FILE             *log_file;
extern struct client    *clients;
extern struct daemon    *daemons;
extern sem_t            daemons_lock;

void*
client_request_list (void *arg) {
    struct client_request   *r;

    r = (struct client_request *)arg;

    extern char **known_files;
    extern int nb_known_files;
    sem_wait (&daemons_lock);
    if (nb_known_files > 0)
        for (int i = 0; i < nb_known_files; i++)
            client_send (r->client, known_files[i]);
    else
        client_send (r->client, " < error list : no known files on network\n");
    sem_post (&daemons_lock);

    return NULL; 
}

