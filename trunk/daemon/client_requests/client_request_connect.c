#include <arpa/inet.h>          // inet_aton ()
#include <netinet/in.h>         // struct sockaddr_in

#include <ctype.h>              // isdigit ()
#include <errno.h>              // errno
#include <stdio.h>              // printf ()
#include <stdlib.h>             // NULL
#include <string.h>             // strchr ()
#include <unistd.h>             // close ()

#include "../../util/logger.h"  // log_failure ()
#include "../client.h"          // client_send ()
#include "../client_request.h"  // struct client_request
#include "../daemon.h"          // daemon_new ()
#include "../util/cmd_parser.h" // cmd_parse ()

extern FILE*            log_file;
extern struct daemon    *daemons;
extern sem_t            daemons_lock;
extern char             my_ip[INET_ADDRSTRLEN];

#define NOT_A_COUPLE           0
#define NO_PORT_SPECIFIED     -1
#define NO_IP_SPECIFIED       -2
#define INVALID_PORT          -3
#define INVALID_IP            -4

static int
is_valid_ip_port (char *foo, int *port)
{
    char *colon;

    colon = strchr (foo, ':');
    if (!colon)
        return NOT_A_COUPLE;

    if (strcmp (colon + 1, "") == 0)
        return NO_PORT_SPECIFIED;

    /* Port */
    unsigned int i;
    for (i = 0; i < strlen (colon+1); ++i)
        if (!isdigit (colon[i+1]))
            return INVALID_PORT;

    *port  = atoi (colon+1);

    /* IP */
    *colon = '\0';
    if (strlen (foo) == 0)
        return NO_IP_SPECIFIED;

    /* TODO : check for ip validity */
    return 1;
}


void*
client_request_connect (void *arg) {
    struct client_request   *r = NULL;
    char                    answer[256];
    int                     daemon_socket = -1;
    struct sockaddr_in      daemon_addr;
    struct daemon           *daemon = NULL;
    struct parsed_cmd       *pcmd = NULL;
    char                    *ip;
    int                     port;

    r = (struct client_request *) arg;
    if (!r)
        return NULL;

    if (cmd_parse_failed ((pcmd = cmd_parse (r->cmd, NULL))))
        return NULL;

    if (pcmd->argc == 0) {
        sprintf (answer, " < connect: Syntax is \"connect IP:PORT\"\n");
        goto send_msg;
    }

    ip = pcmd->argv[1];
    switch (is_valid_ip_port (ip, &port)) {
        case 1:
            break;
        case NOT_A_COUPLE:
            sprintf (answer,
                    " < connect: you must specify an ip and a port\n");
            goto send_msg;
            break;
        case NO_PORT_SPECIFIED:
            sprintf (answer, " < connect: no port specified\n");
            goto send_msg;
            break;
        case NO_IP_SPECIFIED:
            sprintf (answer, " < connect: no ip specified\n");
            goto send_msg;
            break;
        case INVALID_PORT:
            sprintf (answer, " < connect: invalid port\n");
            goto send_msg;
            break;
        case INVALID_IP:
            sprintf (answer, " < connect: invalid ip\n");
            goto send_msg;
            break;
        default:
            sprintf (answer, " < connect: unknown error\n");
            goto send_msg;
            break;
    }

    if (strcmp (ip, my_ip) == 0
        || strcmp (ip, "127.0.0.1") == 0) {
        sprintf (answer, " < connect: can't connect to myself\n");
        goto send_msg;
    }

    /* TODO: We should not connect to an already connected daemon */
    /* TODO: Except if he has timed out ? :d */

    daemon_socket = socket (AF_INET, SOCK_STREAM, 0);
    if (daemon_socket < 0) {
        sprintf (answer,
                 " < connect: could not open a new socket to %s, error: %s\n",
                 ip,
                 strerror (errno));
        goto send_msg;
    }

    // FIXME: inet_atop () ?
    if (inet_pton (AF_INET, ip, &daemon_addr.sin_addr.s_addr) < 0) {
        sprintf (answer, " < connect: couldn't parse IP %s\n", ip);
    }
    daemon_addr.sin_port   = htons (port);
    daemon_addr.sin_family = AF_INET;

    if (connect (daemon_socket,
                (struct sockaddr *) &daemon_addr,
                sizeof (daemon_addr)) < 0) {
        sprintf (answer,
                 " < connect: could not connect to %s:%d, error: %s\n",
                 ip, port, strerror (errno));
        goto send_msg;
    }

    daemon = daemon_new (daemon_socket, ip, port);
    if (!daemon) {
        sprintf (answer, " < connect: could not create a new daemon object");
        goto send_msg;
    }

    sem_wait (&daemons_lock);
    daemons = daemon_add (daemons, daemon);
    sem_post (&daemons_lock);


    sprintf (answer,
            " < connect: connected to %s successfully\n",
            ip);

send_msg:
    if (client_send (r->client, answer) < 0) {
        log_failure (log_file, "cr_connect: client_send () failed");
    }
    if (daemon_socket >= 0)
        close (daemon_socket);
    if (pcmd)
        cmd_parse_free (pcmd);

    return NULL;
}
