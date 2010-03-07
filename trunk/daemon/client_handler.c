#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "client_handler.h"
#include "../util/logger.h"

#define DBG 0
/* Numbers of threads that have been initialized */
static int       n_initialized_threads = 0;
extern FILE* log_file;
static pthread_t threads[MAX_REQUESTS];

static int
is_active_thread (pthread_t *t)
{
    if (!t) {
        return 0;
    }
    return !pthread_kill (*t, 0);
}

static void*
start (void *msg)
{
    log_success (log_file, 
                 " [Threadx] Started new thread (%s) !", 
                (char *)msg);
    sleep (100);
    pthread_detach (pthread_self ());
    return NULL;
}

static int
find_next_available_thread () {
    int i;
    for (i = 0; i < n_initialized_threads; i++)
        if (!is_active_thread (threads+i))
            return i;
    if (i < MAX_REQUESTS) {
        n_initialized_threads += 1;
#if DBG
        fprintf (stdout, "i<MAX and n_init = %d\n", n_initialized_threads);
#endif
        return i;
    }
    return -1;
}


void
handle_request (char *msg) {
//    log_file = fopen ("/tmp/a", "w");
//    log_success (log_file, "handling");
    int i, r;

    i = find_next_available_thread (); 
    if (i == -1) {
        log_failure (log_file, " [Thread] Max threads reached !\n");
        return;
    }
    else {
        r = pthread_create (threads+i, NULL, start, msg);
        if (r < 0)
            log_failure (log_file, "Could not start new thread");
    }
}
