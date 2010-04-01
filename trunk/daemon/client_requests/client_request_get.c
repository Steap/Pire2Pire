#include <stdio.h>              // FILE
#include <string.h>             // strlen ()

#include "../../util/logger.h"  // log_failure ()
#include "../client.h"          // client_send ()
#include "../client_request.h"  // struct client_request
#include "../daemon.h"
#include "../util/cmd.h"        // struct option
#include "../file_cache.h" 

#define HASH_SIZE 32

extern struct daemon *daemons;
extern FILE* log_file;
extern struct file_cache *file_cache;

/* 
 * According to the protocol, there are no options available for the "get"
 * command.
 */
static struct option options[] = {
    {NULL, NULL, 0}    
};

#include <unistd.h>
void*
client_request_get (void *arg) {
    int                     argc;
    char                    **argv;
    int                     c;
    int                     nb_arguments;
    struct client_request   *r;
    char                    key[33];
    struct file_cache       *file_to_dl;

#define MAX_ANSWER_SIZE 256

    char   answer[MAX_ANSWER_SIZE];

    r = (struct client_request *) arg;    
    argv = cmd_to_argc_argv (r->cmd, &argc);
    while ((c = cmd_get_next_option (argc, argv, options)) > 0);
  
    /* 
     *  Checking whether the arguments are valid.  If they are, retrieves the
     *  data (stored in the cache) about the file to be downloaded 
     */ 
    nb_arguments = argc - optind;
    strcpy (key, argv[optind]);
    if (nb_arguments == 0) {
        sprintf (answer,
                 " < get : you need to specify a key.\n");
    }
    else {
        if (strlen (argv[optind]) != HASH_SIZE) {
            sprintf (answer,
                     " < get : hash size is not good, man.\n");
        }
        else {
            file_to_dl = file_cache_get_by_key (file_cache, key);
            if (!file_to_dl) {
                log_failure (log_file,
                             "Could not find the file in the cache");
                goto out;
            }
            sprintf (answer,
                    "< get : seeder is %s ",
                    file_to_dl->seeders->ip_port);
        }
    }

    if (client_send (r->client, answer) < 0) {
        log_failure (log_file, "do_set () : failed to send data to the client");
    }

    /*
     * Retrieving the daemon
     * FIXME : remove that dirty hack.
     * FIXME : use a function in daemon.c
     * Assigned to : roelandt
     */
    struct daemon *d;
    char *addr = strdup (file_to_dl->seeders->ip_port);
    *(strchr (addr, ':')) = '\0';
    for_each_daemon (d) {
        if (strcmp (d->addr, addr) == 0)
            break;
    }    
    if (!d)
        goto out;
   
    log_success (log_file, "OK, got the daemon %s", d->addr); 
    cmd_free (argv);

    /*
     * Sending the "get key begin end" message.
     */ 
    sprintf (answer, 
             "get %s %d %ld\n", 
             file_to_dl->key,
             0,
             file_to_dl->size);
    if (daemon_send (d, answer) < 0) {
        log_failure (log_file, "Could not send the ready command");
        goto out;
    }

    /*
     * Reading the answer ("ready")
     * TODO : what if it never comes ? What if it is not "ready" ?
     * Cmd is supposedly : 
     * ready KEY DELAY IP PORT PROTOCOL BEGINNING END
     */ 
    log_success (log_file, "OK LOL ");
    char readycmd[256];
    if (recv (d->socket, readycmd, 256, 0) < 0) {
        log_failure (log_file,
                     "Error reading the \"ready\" answer");
        goto out;
    }
    log_success (log_file, "Ok %s \n", readycmd);
    

out:
    return NULL;
}
