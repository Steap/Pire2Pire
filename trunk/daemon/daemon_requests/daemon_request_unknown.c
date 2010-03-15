#include <stdio.h>              // FILE

#include "../../util/logger.h"  // log_failure ()
#include "../daemon.h"          // daemon_send ()
#include "../daemon_request.h"  // struct daemon_request

extern FILE *log_file;

void*
daemon_request_unknown (void* arg) {
    struct daemon_request   *r;
    char                    answer[256];
    
    r = (struct daemon_request *) arg;
    if (!r)
        return NULL;

    sprintf (answer, "error %s unknown command\n", r->cmd);
    if (daemon_send (r->daemon, answer) < 0) {
        log_failure (log_file, 
                     "do_unknown_command () : failed to send data back to the \
                     daemon");
        return NULL;
    }
    
    return NULL;
}
