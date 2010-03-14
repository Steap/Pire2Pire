#include <ctype.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "../util/cmd.h"
#include "../client.h"
#include "../client_request.h"

#include "../../util/logger.h"

extern FILE* log_file;

/* 
 * According to the protocol, there are no options available for the "set"
 * command.
 */
#include <unistd.h>

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

    char   answer[MAX_ANSWER_SIZE];

    r = (struct client_request *) arg;    
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
            break;
        case NO_PORT_SPECIFIED:
            sprintf (answer, "No port specified\n");
            break;
        case NO_IP_SPECIFIED:
            sprintf (answer, "No ip specified\n");
            break;
        case INVALID_PORT:
            sprintf (answer, "invalid port\n");
            break;
        case INVALID_IP:
            sprintf (answer, "invalid ip\n");
            break;
        default:
            break;
    }


send_msg:
    if (client_send (r->client, answer) < 0) {
        log_failure (log_file, "do_set () : failed to send data to the client");
    }
    
//out:
    cmd_free (argv);
    sem_wait (&r->client->req_lock);
    r->client->requests = client_request_remove (r->client->requests, r->thread_id);
    sem_post (&r->client->req_lock);
    client_request_free (r);
    pthread_detach (pthread_self ());
    return NULL;

}
