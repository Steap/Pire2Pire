#include <arpa/inet.h>          // inet_aton ()
#include <netinet/in.h>         // struct sockaddr_in

#include <ctype.h>              // isdigit ()
#include <errno.h>              // errno
#include <stdio.h>              // printf ()
#include <stdlib.h>             // NULL
#include <string.h>             // strchr ()

#include "../../util/logger.h"  // log_failure ()
#include "../client.h"          // client_send ()
#include "../client_request.h"  // struct client_request
#include "../daemon.h"          // daemon_new ()
#include "../util/cmd.h"        // struct option

extern FILE* log_file;
extern struct daemon *daemons;
extern sem_t daemons_lock;

#define NOT_A_COUPLE           0
#define NO_PORT_SPECIFIED     -1
#define NO_IP_SPECIFIED       -2
#define INVALID_PORT          -3
#define INVALID_IP            -4

struct option options[] = {
    {NULL, NULL, 0}
};
static int 
is_valid_ip_port (char *foo, int *port)
{
    // FIXME: printf () !?
    printf ("FOO IS %s\n", foo);
    char *sep;

    sep = strchr (foo, ':');
    if (!sep)
        return NOT_A_COUPLE;

    if (strcmp (sep+1, "") == 0)
        return NO_PORT_SPECIFIED;

    /* Port */
    unsigned int i;
    for (i = 0; i < strlen (sep+1); ++i)
        if (!isdigit (sep[i+1]))
            return INVALID_PORT;
                                                                                        
    *port  = atoi (sep+1);

    /* IP */
    *sep = '\0';
    if (strlen (foo) == 0)
        return NO_IP_SPECIFIED;

    /* TODO : check for ip validity */
    return 1;
}


void*
client_request_connect (void *arg) {
    int                     argc;
    char                    **argv;
    int                     c;
    int                     nb_arguments;
    struct client_request   *r;

#define MAX_ANSWER_SIZE 256

    char                    answer[MAX_ANSWER_SIZE];
    int                     daemon_socket;
    struct sockaddr_in      daemon_addr;
    struct daemon           *daemon;

    r = (struct client_request *) arg;
    if (!r)
        return NULL;
    argv = cmd_to_argc_argv (r->cmd, &argc);
    while ((c = cmd_get_next_option (argc, argv, options)) > 0);

    nb_arguments = argc - optind;
    if (nb_arguments == 0) {
        sprintf (answer, "connect : you should specify an ip and a port.\n");
        goto send_msg;
    }

    int port;
    char *addr = argv[optind];
    switch (is_valid_ip_port (addr, &port)) {
        case 1:
            sprintf (answer, 
                     "connect : should connect to %s, port %d.\n",
                     addr,
                     port);
            break;
        case NOT_A_COUPLE:
            sprintf (answer, "You must specify an ip and a port.\n");
            goto send_msg;
            break;
        case NO_PORT_SPECIFIED:
            sprintf (answer, "No port specified\n");
            goto send_msg;
            break;
        case NO_IP_SPECIFIED:
            sprintf (answer, "No ip specified\n");
            goto send_msg;
            break;
        case INVALID_PORT:
            sprintf (answer, "invalid port\n");
            goto send_msg;
            break;
        case INVALID_IP:
            sprintf (answer, "invalid ip\n");
            goto send_msg;
            break;
        default:
            goto send_msg;
            break;
    }

    /*
     * TODO: We should not connect to ourselves, check the IP with my_ip
     * and with 127.0.0.1 at least
     */

    daemon_socket = socket (AF_INET, SOCK_STREAM, 0);
    if (daemon_socket < 0) {
        sprintf (answer,
                 "Could not open a new socket to %s. Error : %s\n",
                 addr,
                 strerror (errno));
        goto send_msg;
    }

    // FIXME: inet_atop () ?
    inet_aton (addr, &daemon_addr.sin_addr);
    daemon_addr.sin_port   = htons (port);
    daemon_addr.sin_family = AF_INET;

    if (connect (daemon_socket,
                (struct sockaddr *) &daemon_addr,
                sizeof (daemon_addr)) < 0) {    
        sprintf (answer, 
                 "Could not connect to %s:%d.\n Error : %s.\n",
                 addr, port, strerror (errno));
        goto send_msg;
    }

    daemon = daemon_new (daemon_socket, addr);
    if (!daemon) {
        sprintf (answer, "Could not create a new daemon object");
        goto send_msg;
    }

    log_failure (log_file, "connect : before sem_wait");

#define SEM 1
#if SEM
    sem_wait (&daemons_lock);
#endif
    daemons = daemon_add (daemons, daemon);
    log_failure (log_file, "connect : after add");
#if SEM
    sem_post (&daemons_lock);
#endif

    log_failure (log_file, "connect : after post");
  
    sprintf (answer, "Connected to %s. Everything went fine.\n", addr); 
     

send_msg:
    if (client_send (r->client, answer) < 0) {
        log_failure (log_file, "do_set () : failed to send data to the client");
    }
    
    cmd_free (argv);

    return NULL;
}
