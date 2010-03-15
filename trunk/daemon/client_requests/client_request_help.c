#include <stdlib.h>             // NULL

#include "../client.h"          // client_send ()
#include "../client_request.h"  // struct client_request

#define HELP_TEXT   \
"\
====================================HELP====================================\n\
connect IP:PORT     Connects the daemon to another daemon                   \n\
get KEY             Has your daemon download the file whose key is KEY      \n\
help                Displays this message                                   \n\
info                Displays info about the daemon state                    \n\
list                Displays a list of available files on the network       \n\
set                 Lists available options                                 \n\
set option=value    Sets an option's value                                  \n\
quit                Quits                                                   \n\
============================================================================\n"

void*
client_request_help (void *arg) {
    struct client_request   *r;

    r = (struct client_request *) arg;
    if (!r)
        return NULL;

    client_send (r->client, HELP_TEXT);

    return NULL; 
}

