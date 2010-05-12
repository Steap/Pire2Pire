#include <stdio.h>              // FILE

#include "../../util/logger.h"  // log_failure ()
#include "../daemon.h"          // daemon_send ()
#include "../daemon_request.h"  // struct daemon_request

extern FILE *log_file;

void*
daemon_request_unknown (void* arg) {
/*
 * This currently does nothing, because of an annoying bug.
 *
 * Two machines : Zach & Lynette.
 *
 * Lynette tries to connect to Zach using its client port.
 * Zach receives a "neighbour" command, but thinks it is a client command.
 * Zach then sends back a "Unknwon command" message to Lynette, that is a client
 * to him.
 * Lynette receives this message, and since it is not a valid daemon command,
 * sends back an "error" message.
 * This goes on for ever, causing both daemon to crash.
 *
 * The easiest and dmbest solution is to never send error messages.
 *
 * FIXME : do something clever.
 * Marked as Workaround. 
 */
#if 0
    struct daemon_request   *r;
    char                    answer[256];

    r = (struct daemon_request *) arg;
    if (!r)
        return NULL;

    sprintf (answer, "error %s is an unknown command.\n\n", r->cmd);
    log_send (log_file, answer);
    if (daemon_send (r->daemon, answer) < 0) {
        log_failure (log_file,
                     "do_unknown_command () : failed to send data back to the \
                     daemon");
        return NULL;
    }
#endif
    return NULL;
}
