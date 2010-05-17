/*
 * When receiving the "neighbour command" from another daemon, this daemon
 * should try and add the given IP/port to his list of known daemons.
 */
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../conf.h"
#include "../daemon.h"
#include "../daemon_handler.h"
#include "../daemon_requests.h"
#include "../util/logger.h"
#include "../util/socket.h"

extern sem_t                daemons_lock;
extern struct daemon        *daemons;
extern FILE                 *log_file;

void*
daemon_request_neighbour (void *arg) {
    struct daemon_request   *r;
    struct daemon           *tmp;
    char                    *ip;
    int                     port;

    r = (struct daemon_request *) arg;
    if (!r) {
        log_failure (log_file,
                     "The argument of daemon_request_neighbour seems to be \
                     corrupted");
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
        ip = ipport;
    }

    sem_wait (&daemons_lock);
    /* Checking whether the daemon is used */
    for_each_daemon (tmp)
        if (strcmp (tmp->addr, ip) == 0)
            goto out;
/*
 * FIXME: COPIED FROM client_request_connect, NEED CHANGES
 */
    {
        /* Daemon is not already knwon : let's add it */
        extern struct pool      *daemons_pool;
        extern struct prefs     *prefs;
        extern char             my_ip[INET_ADDRSTRLEN];
        int                     daemon_socket;
        struct sockaddr_in      daemon_addr;
        struct daemon           *daemon = NULL;

        if (strcmp (ip, my_ip) == 0
            || strcmp (ip, "127.0.0.1") == 0) {
            log_failure (log_file, "dr_neighbour: fail 1");
            goto out;
        }

        /* TODO: We should not connect to an already connected daemon */
        /* TODO: Except if he has timed out ? :d */

        daemon_socket = socket (AF_INET, SOCK_STREAM, 0);
        if (daemon_socket < 0) {
            log_failure (log_file, "dr_neighbour: fail 2");
            goto out;
        }

        if (inet_pton (AF_INET, ip, &daemon_addr.sin_addr.s_addr) < 0) {
            log_failure (log_file, "dr_neighbour: fail 3");
        }
        daemon_addr.sin_port   = htons (port);
        daemon_addr.sin_family = AF_INET;

        if (connect (daemon_socket,
                    (struct sockaddr *) &daemon_addr,
                    sizeof (daemon_addr)) < 0) {
            log_failure (log_file, "dr_neighbour: fail 4");
            goto out;
        }

        /*
         * Now we must identify so that the daemon at the other side won't
         * close the connection
         */
        char ident_msg[256];
        sprintf (ident_msg,
                "neighbour %s:%d\n",
                my_ip,
                prefs->daemon_port);
        socket_sendline (daemon_socket, ident_msg);

        daemon = daemon_new (daemon_socket, ip, port);
        if (!daemon) {
            goto out;
        }

        log_success (log_file, "CONNECT     daemon %s", daemon->addr);

        pool_queue (daemons_pool, handle_daemon, daemon);
    }

/* OLD WAY :
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
*/

out:
    sem_post (&daemons_lock);
    return NULL;
}
