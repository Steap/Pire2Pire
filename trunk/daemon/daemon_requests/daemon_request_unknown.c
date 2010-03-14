#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../daemon.h"
#include "../daemon_request.h"

#include "../../util/logger.h"

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
