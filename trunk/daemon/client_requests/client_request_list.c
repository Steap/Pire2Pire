#include <sys/stat.h>           // entry_stat

#include <arpa/inet.h>          // inet_pton () 

#include <dirent.h>             // DIR
#include <stdio.h>              // NULL
#include <stdlib.h>             // malloc ()
#include <unistd.h>             // close ()

#include "../../util/md5/md5.h" // MDFile ()
#include "../../util/logger.h"  // log_failure ()
#include "../client.h"          // client_send ()
#include "../client_request.h"  // struct client_request
#include "../file_cache.h"        // file_tree_to_client ()
#include "../conf.h"
#include "../daemon.h"          // struct daemon
#include "../util/cmd.h"        // cmd_to_argc_argv ()
//#include "../util/cmd_parser.h" // cmd_parse ()
#include "../util/socket.h"     // socket_sendline ()

extern struct prefs *prefs;
//#define SHARED_FOLDER "/tmp/lol/"
#define SHARED_FOLDER prefs->shared_folder

extern FILE                 *log_file;
extern struct file_cache    *file_cache;
extern sem_t                file_cache_lock;
extern struct daemon        *daemons;
extern sem_t                daemons_lock;

void*
client_request_list (void *arg) {
/* cmd_parser version: */
#if 0
    static struct option_template options[] = {
        {0, NULL, 0}
    };
#endif
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
/* cmd version: */
    int                     argc;
    char                    **argv;
/* cmd_parser version: */
#if 0
    struct parsed_cmd       *pcmd;
    struct arg_list         *arg_list;
#endif

    r = (struct client_request *)arg;
    if (!r)
        return NULL;

    // First we must make a big list of sockets connected to all daemons
    sem_wait (&daemons_lock);
    nb_daemons = daemon_numbers (daemons);
    sockets = (int *)malloc (nb_daemons * sizeof (int));
    d = daemons;
    // For each daemon, we will try to initialize and connect a socket
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
        d_addr.sin_port = htons (d->port);
        if (connect (sockets[i],
            (struct sockaddr *)&d_addr,
            sizeof (d_addr)) < 0) {
            log_failure (log_file,
                    "client_request_list (): Couldn't connect to a daemon");
            close (sockets[i]);
            sockets[i] = -1;
        }
    }
    sem_post (&daemons_lock);

    nfds = 0;
    // Then we send a list message to each of them
    for (int i = 0; i < nb_daemons; i++) {
        if (sockets[i] >= 0) {
            socket_sendline (sockets[i], "list\n");
        }

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
    // We will recreate the file_cache tree
    sem_wait (&file_cache_lock);
    file_cache_free (file_cache);
    file_cache = NULL;
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
                        /*
                         * response is supposedly:
                         * file_cache NAME KEY SIZE IP:PORT
                         */
/* cmd version: */
                        argv = cmd_to_argc_argv (response, &argc);
                        if (argc != 5) {
                            cmd_free (argv);
                            free (response);
                            continue;
                        }
                        /* FIXME: not 2 and 4 if there are options... */
                        file_cache = file_cache_add (file_cache,
                                                        argv[1],
                                                        argv[2],
                                                        atoi (argv[3]),
                                                        argv[4]);
                        cmd_free (argv);
/* cmd_parser version: */
#if 0
                        pcmd = cmd_parse (response, options);
                        /* If it is not parsed, we don't forward it */
                        if (pcmd == PARSER_MISSING_ARGUMENT
                            || pcmd == PARSER_UNKNOWN_OPTION
                            || pcmd == PARSER_EMPTY_COMMAND) {
                            free (response);
                            continue;
                        }
                        /* We skip NAME */
                        arg_list = pcmd->arguments->next;
                        /* arg_list points to KEY */
                        file_caches = file_tree_add (files,
                                                /* KEY */
                                                arg_list->text,
                                                /* IP:PORT */
                                                arg_list->next->next->text);

                        cmd_parse_free (pcmd);
                        /* Just in case: */
                        key = NULL;
                        ip_port = NULL;
                        pcmd = NULL;
                        arg_list = NULL;
#endif

                        client_send (r->client, " < ");
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
    sem_post (&file_cache_lock);

    // And close the sockets properly
    for (int i = 0; i < nb_daemons; i++) {
        if (sockets[i] >=0) {
            socket_sendline (sockets[i], "quit\n");
            close (sockets[i]);
        }
    }

    free (sockets);

    return NULL; 
}

