/*
 * When receiving the "neighbour command" from another daemon, this daemon
 * should try and add the given IP/port to his list of known daemons.
 */
#include <sys/socket.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../daemon.h"
#include "../daemon_requests.h"

#include "../../util/logger.h"


extern sem_t                daemons_lock;
extern struct daemon        *daemons;
extern FILE                 *log_file;

void*
daemon_request_neighbour (void *arg) {
    struct daemon_request   *r;
    struct daemon           *tmp;
    struct daemon           *new_daemon;
    char                    *addr;
    int                     port;
    int                     new_daemon_socket;

     
    r = (struct daemon_request *) arg;
    if (!r) {
        log_failure (log_file, 
                     "The argument of daemon_request_neighbour seems to be corrupted");
        return NULL;
    }


    /*
     * Ugly hack since we do not know what parser to use.
     */
    {
        char *ipport;
        ipport = strchr (r->cmd, ' ') +1;
        port = atoi (1 + strchr (ipport, ':'));
        *(strchr (ipport, ':')) = '\0';
        addr = ipport;
    }

    sem_wait (&daemons_lock);
    /* Checking whether the daemon is used */
    for (tmp = daemons; tmp; tmp = tmp->next)
        if (strcmp (tmp->addr, addr) == 0)
            goto out;
    
    /* Daemon is not already knwon : let's add it */
    new_daemon_socket = socket (AF_INET, SOCK_STREAM, 0);
    if (new_daemon_socket < 0) {
        log_failure (log_file,
                    "Could not add %s to the list of known daemons (%s)",
                    addr,
                    strerror (errno));
        goto out;
    }
    new_daemon = daemon_new (new_daemon_socket, addr, port);
    if (!new_daemon)
        goto out;
    daemons = daemon_add (daemons, new_daemon);

out:
    sem_post (&daemons_lock);
    return NULL;
}
