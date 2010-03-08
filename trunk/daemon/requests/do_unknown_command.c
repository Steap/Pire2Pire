#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "../callback_argument.h"

#include "../../util/logger.h"


extern FILE *log_file;

void*
do_unknown_command (void* arg) {
    char                      answer[256];
    struct callback_argument  *cba;
    
    cba = (struct callback_argument *) arg;
    if (!cba)
        return NULL;

    sprintf (answer, " < Unknown command '%s'\n", cba->cmd);
    if (send (cba->client_socket, answer, strlen (answer), 0) < 0) {
        log_failure (log_file, 
                     "do_unknown_command () : failed to send data back to the \
                     client");
        return NULL;
    }

    if (cba)
        cba_free (cba);
    return NULL;
}
