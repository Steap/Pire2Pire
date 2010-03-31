#include <stdio.h>
#include <stdlib.h>

#include "../daemon.h"
#include "../daemon_request.h"
/*
 * When receiving the "neighbourhood" command from another daemon, this daemon
 * should send back length(daemons) many "neighbour" commands.
 *
 * Example :
 *
 * D1 sends "neighbourhood" to D2.
 * D2 knows D3, D4, D5.
 * D2 answers :
 *     neighbour D3.ip
 *     neighbour D4.ip
 *     neighbour D5.ip
 */

extern sem_t                  daemons_lock;
extern struct daemon          *daemons;

void*
daemon_request_neighbourhood (void *arg) {
    struct daemon_request    *r;
    struct daemon            *tmp;
    char                     answer[25]; // FIXME : LOLZ

    r = (struct daemon_request *) arg;
    if (!r)
        return NULL;

    sem_wait (&daemons_lock);

    for_each_daemon (tmp) {
        sprintf (answer, "neighbour %s\n", tmp->addr);
        daemon_send (r->daemon, answer);
    }

    sem_post (&daemons_lock);

    return NULL;  
}
