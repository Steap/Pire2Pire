#include <sys/stat.h>           // entry_stat

#include <arpa/inet.h>          // inet_pton () 

#include <stdio.h>              // NULL
#include <stdlib.h>             // malloc ()
#include <dirent.h>             // DIR
#include <unistd.h>             // close ()

#include "../../util/md5/md5.h" // MDFile ()
#include "../../util/logger.h"  // log_failure ()
#include "../client.h"          // client_send ()
#include "../client_request.h"  // struct client_request
//#include "../resource.h"        // resource_tree_to_client ()
#include "../conf.h"
#include "../daemon.h"          // struct daemon
#include "../util/socket.h"     // socket_sendline ()

extern struct prefs *prefs;
//#define SHARED_FOLDER "/tmp/lol/"
#define SHARED_FOLDER prefs->shared_folder

extern FILE             *log_file;
//extern struct resource_tree     *resources;
//extern sem_t                    resources_lock;
extern struct daemon    *daemons;
extern sem_t            daemons_lock;

void*
client_request_list (void *arg) {
    struct client_request   *r;
    struct daemon           *d;
    struct sockaddr_in      d_addr;
    int                     nb_daemons;
    int                     *sockets;
    fd_set                  sockets_set;
    int                     nfds;
    struct timeval          timeout;
    int                     select_value;
    char                    *response;

    r = (struct client_request *)arg;
    if (!r)
        return NULL;

    // First we must make a big list of sockets connected to all daemons
    sem_wait (&daemons_lock);
    nb_daemons = daemon_numbers (daemons);
    sockets = (int *)malloc (nb_daemons * sizeof (int));
    d = daemons;
    for (int i = 0; i < nb_daemons; i++, d = d->next) {
        sockets[i] = socket (AF_INET, SOCK_STREAM, 0);
        if (sockets[i] < 0) {
            log_failure (log_file,
                    "client_request_list (): Failed to create a socket");
            continue;
        }
        // TODO: IPv6
        if (inet_pton (AF_INET, d->addr, &d_addr.sin_addr) <= 0) {
            log_failure (log_file,
                    "client_request_list (): Failed to convert daemon addr");
            continue;
        }
        d_addr.sin_family = AF_INET;    // FIXME: d should have a sockaddr_in
        d_addr.sin_port = htons (7331); // FIXME: d should have a sockaddr_in
        if (connect (sockets[i],
            (struct sockaddr *)&d_addr,
            sizeof (d_addr)) < 0) {
            log_failure (log_file,
                    "client_request_list (): Couldn't connect to a daemon");
            close (sockets[i]);
            sockets[i] = 0;
        }
    }
    sem_post (&daemons_lock);

    nfds = 0;
    // Then we send a list message to each of them
    for (int i = 0; i < nb_daemons; i++) {
        socket_sendline (sockets[i], "list");

        // While we're at it, we prepare nfds
        if (sockets[i] > nfds)
            nfds = sockets[i];
    }
    // nfds is the max of socket descriptors + 1
    nfds++;

    // Finally, we select () the sockets and client_send () responses
    FD_ZERO (&sockets_set);
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;
    for (int i = 0; i < nb_daemons; i++) {
        if (sockets[i] >= 0)
            FD_SET (sockets[i], &sockets_set);
    }
    for (;;) {
        select_value = select (nfds, &sockets_set, NULL, NULL, &timeout);
        if (select_value < 0) {
            log_failure (log_file, "client_request_list (): select failed");
            break;
        }
        else if (!select_value) {
            // select_value == 0 on timeout
            break;
        }
        else {
            for (int i = 0; i < nb_daemons; i++) {
                if (sockets[i] >= 0) {
                    if (FD_ISSET (sockets[i], &sockets_set)) {
                        response = socket_getline_with_trailer (sockets[i]);
                        client_send (r->client, response);
                        free (response);
                    }
                    else {
                        // We put it back for next loop
                        FD_SET (sockets[i], &sockets_set);
                    }
                }
            }
        }
    }

    // And close the sockets properly
    for (int i = 0; i < nb_daemons; i++) {
        if (sockets[i] >=0) {
            close (sockets[i]);
        }
    }

    client_send (r->client, "END OF FILES\n");

    return NULL; 
}

/* With resource.c
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
*/
