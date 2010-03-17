#include <sys/stat.h>           // entry_stat

#include <stdio.h>              // NULL
#include <stdlib.h>             // malloc ()
#include <dirent.h>             // DIR

#include "../../util/md5/md5.h" // MDFile ()
#include "../../util/logger.h"  // log_failure ()
#include "../client.h"          // client_send ()
#include "../client_request.h"  // struct client_request
#include "../resource.h"        // resource_tree_to_client ()
#include "../conf.h"

extern struct prefs *prefs;
//#define SHARED_FOLDER "/tmp/lol/"
#define SHARED_FOLDER prefs->shared_folder

extern FILE                     *log_file;
extern struct resource_tree     *resources;
extern sem_t                    resources_lock;

void*
client_request_list (void *arg) {
    struct client_request   *r;

    r = (struct client_request *)arg;

    sem_wait (&resources_lock);
    if (resources)
        resource_tree_to_client (resources, r->client);
    else
        client_send (r->client, " < error list : no known files on network\n");
    sem_post (&resources_lock);

    return NULL; 
}

