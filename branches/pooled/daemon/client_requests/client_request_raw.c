#include <pthread.h>            // pthread_t
#include <string.h>              // strncmp ()

#include "../util/logger.h"  // log_failure ()
#include "../util/cmd.h"        // struct option
#include "../client.h"          // client_send ()
#include "../client_request.h"  // struct client_request
#include "../daemon.h"          // struct daemon

extern FILE             *log_file;
extern struct daemon    *daemons;
extern sem_t            daemons_lock;
extern sem_t            clients_lock;

static struct option options[] = {
    {NULL, NULL, 0}
};

void*
client_request_raw (void *arg) {
    struct client_request   *r;
    int                     argc;
    char                    **argv;
    int                     c;
    int                     nb_arguments;
    int                     trusted_source;
    struct daemon           *d;
    char                    *semicolon;
    int                     d_port;
    char                    *cmd;

    r = (struct client_request *) arg;
    if (!r)
        return NULL;
    argv = cmd_to_argc_argv (r->cmd, &argc);
    while ((c = cmd_get_next_option (argc, argv, options)) > 0);

    nb_arguments = argc - optind;
    if (nb_arguments < 2) {
        client_send (r->client, " < Syntax: raw IP:PORT CMD\n");
        return NULL;
    }

    trusted_source = 0;
    sem_wait (&clients_lock);
    if (strncmp ("127.0.0.1", r->client->addr, 9) == 0) {
        trusted_source = 1;
    }
    sem_post (&clients_lock);
    if (!trusted_source) {
        client_send (r->client, " < raw: Sorry, I only trust 127.0.0.1\n");
        return NULL;
    }

    // Search for the connected daemon at requested address (argv[1])
    semicolon = strchr (argv[1], ':');
    if (!semicolon) {
        client_send (r->client, " < Syntax: raw IP:PORT CMD\n");
        return NULL;
    }
    *semicolon = '\0'; // so that argv[1] equals the daemon IP
    // Here, semicolon + 1 points to the port part of argv[1]
    if (sscanf (semicolon + 1, "%d", &d_port) < 1) {
        client_send (r->client, " < Syntax: raw IP:PORT CMD\n");
        return NULL;
    }

    sem_wait (&daemons_lock);
    /********************************************************
    WATCH OUT : DO NOT USE GOTO BETWEEN SEM_WAIT AND SEM_POST
    ********************************************************/
    // Searching for the right daemon in daemons
    d = daemons;
    while (d) {
        if ((strcmp (argv[1], d->addr) == 0) && d_port == d->port)
            break;
        d = d->next;
    }
    if (d) {
        cmd = strchr (r->cmd, ' ');
        /*
         *                cmd
         *                 v
         * r->cmd   >   raw IP:PORT CMD OPT1 OPT2 ...
         */
        cmd = strchr (cmd + 1, ' ');
        /*
         *                        cmd
         *                         v
         * r->cmd   >   raw IP:PORT CMD OPT1 OPT2 ...
         *                          ^
         *                       cmd + 1
         */
        daemon_send (d, cmd + 1);
        daemon_send (d, "\n");

        client_send (r->client, " < raw: Sent raw command successfully\n");
    }
    else {
        client_send (r->client,
                " < raw: Please use connect before sending raw commands\n");
    }
    sem_post (&daemons_lock);

    return NULL;
}

