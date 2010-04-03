#include <semaphore.h>          // sem_t
#include <stdio.h>              // FILE
#include <string.h>             // strcpy ()

#include "../../util/logger.h"  // log_failure ()
#include "../daemon.h"          // daemon_send ()
#include "../daemon_request.h"  // struct daemon_request
#include "../file_cache.h"      // struct file_cache
#include "../util/cmd_parser.h" // cmd_parse ()

extern FILE                     *log_file;
extern struct file_cache        *file_cache;
extern sem_t                    file_cache_lock;
extern struct client            *list_client;

void*
daemon_request_file (void* arg) {
    struct daemon_request   *r;
    struct parsed_cmd       *pcmd;
    struct client           *lister;

    r = (struct daemon_request *) arg;
    if (!r)
        return NULL;

    /*
     * First, did someone asked "list"?
     */
    lister = list_client;
    if (!lister)
        return NULL;

    /*
     * cmd is supposedly:
     * file NAME KEY SIZE IP:PORT
     *  0    1    2   3      4
     */
    if (cmd_parse_failed ((pcmd = cmd_parse (r->cmd, NULL)))) {
        return NULL;
    }
    if (pcmd->argc < 5) {
        cmd_parse_free (pcmd);
        return NULL;
    }

    /*
     * Hackety hack:
     * IP:PORT\0   ->   IP\0PORT\0
     */
    char *port = strchr (pcmd->argv[4], ':');
    *port = '\0';
    port++;
    file_cache = file_cache_add (file_cache,
                                pcmd->argv[1],
                                pcmd->argv[2],
                                atol (pcmd->argv[3]),
                                pcmd->argv[4],
                                atoi (port));
    cmd_parse_free (pcmd);
    char *answer = (char *)malloc ((strlen (r->cmd) + 5) * sizeof (char));
    sprintf (answer,
            " < %s\n",
            r->cmd);
    client_send (lister, answer);
    free (answer);

    return NULL;
}
