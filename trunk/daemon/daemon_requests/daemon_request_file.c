#include <stdio.h>              // FILE
#include <string.h>             // strcpy ()

#include "../../util/logger.h"  // log_failure ()
#include "../daemon.h"          // daemon_send ()
#include "../daemon_request.h"  // struct daemon_request

extern FILE *log_file;

#define MAX_KNOWN_FILES     1024
char    **known_files = NULL;
int     nb_known_files = 0;

void*
daemon_request_file (void* arg) {
    struct daemon_request   *r;
    int                     size;
    
    r = (struct daemon_request *) arg;
    if (!r)
        return NULL;

    if (!known_files) {
        known_files = (char **)malloc (MAX_KNOWN_FILES * sizeof (char *));
        if (!known_files)
            return NULL;
    }

    if (nb_known_files < MAX_KNOWN_FILES) {
        size = strlen (r->cmd) + 1;
        known_files[nb_known_files] = (char *)malloc (size * sizeof (char));
        strcpy (known_files[nb_known_files], r->cmd);
        nb_known_files++;
    }
 
    return NULL;
}
