#include <stdio.h>              // FILE
#include <string.h>             // strcpy ()

#include "../../util/logger.h"  // log_failure ()
#include "../daemon.h"          // daemon_send ()
#include "../daemon_request.h"  // struct daemon_request
//#include "../resource.h"        // struct resource_tree

#define STR(x)   #x
#define XSTR(x)  STR(x)

extern FILE                     *log_file;
//extern struct resource_tree     *resources;
//extern sem_t                    resources_lock;

void*
daemon_request_file (void* arg) {
    struct daemon_request   *r;
/*
    struct resource         *res;
    char                    *answer;
    char                    filename[RESOURCE_FILENAME_SIZE + 1];
    char                    key[RESOURCE_KEY_SIZE + 1];
    unsigned long long int  size;
    char                    ip_port[RESOURCE_IP_PORT_SIZE + 1];
*/

    r = (struct daemon_request *) arg;
    if (!r)
        return NULL;

    log_failure (log_file,
                "daemon_request_file (): Received \"file\" out of nowhere");
/*
    // FIXME: use cmd_to_argc_argv instead...
    if (sscanf (r->cmd, "file %"
                        XSTR (RESOURCE_FILENAME_SIZE)
                        "s %"
                        XSTR (RESOURCE_KEY_SIZE)
                        "s %llu %"
                        XSTR (RESOURCE_IP_PORT_SIZE)
                        "s\n",
                        filename, key, &size, ip_port) < 4) {
        answer = (char *)malloc ((strlen (r->cmd) + 36) * sizeof (char));
        sprintf (answer, "error %s: Unable to read the command\n", r->cmd);
        daemon_send (r->daemon, answer);
        return NULL;
    }

    sem_wait (&resources_lock);
    res = resource_tree_get_by_key (resources, key);
    if (!res) {
        res = resource_new (filename, key, size, ip_port);
        resources = resource_tree_add (resources, res);
    }
    sem_post (&resources_lock);
*/

    return NULL;
}
