#include <sys/stat.h>           // entry_stat

#include <arpa/inet.h>          // inet_pton ()

#include <dirent.h>             // DIR
#include <stdio.h>              // NULL
#include <stdlib.h>             // malloc ()
#include <string.h>             // strchr ()
#include <unistd.h>             // close ()

#include "../util/md5/md5.h" // MDFile ()
#include "../util/logger.h"  // log_failure ()
#include "../client.h"          // client_send ()
#include "../client_request.h"  // struct client_request
#include "../file_cache.h"      // struct file_cache
#include "../conf.h"            // struct prefs
#include "../daemon.h"          // struct daemon
#include "../util/cmd_parser.h" // cmd_parse ()
#include "../util/socket.h"     // socket_sendline ()

/* How long should a client be redirected file messages */
#define LIST_TIMEOUT    25

extern struct prefs *prefs;
#define SHARED_FOLDER prefs->shared_folder

extern FILE                 *log_file;
extern struct file_cache    *file_cache;
extern sem_t                file_cache_lock;
extern struct daemon        *daemons;
extern sem_t                daemons_lock;
extern struct client        *list_client;
extern sem_t                list_lock;

/*
static int                      nb_daemons;
static int                      *sockets;
static int                      nfds;
static struct client_request    *r;

static void create_socket_table ();
static void send_list_to_each_socket ();
static void forward_responses ();
static void free_socket_table ();
*/

void*
client_request_list (void *arg) {
    struct client_request   *r;
    struct daemon           *d;

    r = (struct client_request *)arg;
    if (!r)
        return NULL;

    /* Lock list */
    sem_wait (&list_lock);
    list_client = r->client;

    sem_wait (&daemons_lock);
    for_each_daemon (d) {
        daemon_send (d, "list\n");
    }
    sem_post (&daemons_lock);

    /* For 1 second, I shall be forwarded the responses by dr_file */
//    sleep (LIST_TIMEOUT);

    /* Release list */
//    list_client = NULL;
    sem_post (&list_lock);

    log_failure (log_file, "******** End of list ********");
    /*
     * Here, we will create a socket for each daemon, in order to broadcast a
     * "list" message, and forward all "file" responses to the client who asked
     * for the list.
     */
/*
    create_socket_table ();

    send_list_to_each_socket ();

    forward_responses ();

    free_socket_table ();
*/

    return NULL;
}

#if 0
static void
create_socket_table () {
    struct daemon           *d;
    struct sockaddr_in      d_addr;

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
        /* TODO: IPv6 */
        if (inet_pton (AF_INET, d->addr, &d_addr.sin_addr) <= 0) {
            log_failure (log_file,
                    "client_request_list (): Failed to convert daemon addr");
            continue;
        }
        /* FIXME: d should have a sockaddr_in field */
        d_addr.sin_family = AF_INET;
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
}

static void
send_list_to_each_socket () {
    nfds = 0;
    /* Then we send a list message to each of them */
    for (int i = 0; i < nb_daemons; i++) {
        if (sockets[i] >= 0) {
            socket_sendline (sockets[i], "list\n");
        }

        /* While we're at it, we prepare nfds */
        if (sockets[i] > nfds)
            nfds = sockets[i];
    }
    nfds++;
}

static void
forward_responses () {
    char                    *response;
    int                     select_value;
    struct timeval          timeout;
    fd_set                  sockets_set;
    struct option_template  options[] = {
        {0, NULL, NO_ARG}
    };
    struct parsed_cmd       *pcmd;

    /* Finally, we select () the sockets and client_send () responses */
    FD_ZERO (&sockets_set);
    for (int i = 0; i < nb_daemons; i++) {
        if (sockets[i] >= 0)
            FD_SET (sockets[i], &sockets_set);
    }
    /* We will recreate the file_cache tree */
    sem_wait (&file_cache_lock);
    file_cache_free (file_cache);
    file_cache = NULL;
    for (;;) {
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
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
                        /* If it is not parsed, we don't forward it */
                        if (cmd_parse_failed (pcmd = cmd_parse (response,
                                                                options))) {
                            free (response);
                            continue;
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
}

static void
free_socket_table () {
    for (int i = 0; i < nb_daemons; i++) {
        if (sockets[i] >=0) {
            socket_sendline (sockets[i], "quit\n");
            close (sockets[i]);
        }
    }

    free (sockets);
}

/* TODO: remove this if it is unneeded
                        argv = cmd_to_argc_argv (response, &argc);
                        if (argc != 5) {
                            log_failure (log_file,
                            "cr_list forward_responses (): Unable to parse %s",
                                        response);
                            cmd_free (argv);
                            free (response);
                            continue;
                        }
                        file_cache = file_cache_add (file_cache,
                                                        argv[1],
                                                        argv[2],
                                                        atoi (argv[3]),
                                                        argv[4]);
                        cmd_free (argv);
*/
#endif
