/*
 * A daemon might answer that an error occurred (the file it was asked for does
 * not exist, for instance).
 *
 * Upon receiving an error message, the daemon should warn the client that their
 * request cannot be performed.
 */
#include <sys/stat.h>

#include <netinet/in.h>

#include <stdio.h>
#include <stdlib.h>

#include "../daemon.h"
#include "../daemon_request.h"
#include "../../util/logger.h"



extern FILE *log_file;

void*
daemon_request_error (void *arg) {
    struct daemon_request *r;

    r = (struct daemon_request *) arg;
    if (!r)
        return NULL;

    log_failure (log_file,
                 "ERROR %s:%d returned the following error",
                 r->daemon->addr,
                 r->daemon->port);

    return NULL;
}
