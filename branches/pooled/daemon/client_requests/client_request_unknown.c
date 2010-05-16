#include "../util/logger.h"  // log_failure ()
#include "../client.h"          // client_send ()
#include "../client_request.h"  // struct client_request

extern FILE *log_file;

void*
client_request_unknown (void* arg) {
    struct client_request   *r;
    char                    answer[256];

    r = (struct client_request *) arg;
    if (!r)
        return NULL;

    sprintf (answer, " < Unknown command '%s'\n", r->cmd);
    if (client_send (r->client, answer) < 0) {
        log_failure (log_file,
                     "do_unknown_command () : failed to send data back to the \
                     client");
        return NULL;
    }

    return NULL;
}
