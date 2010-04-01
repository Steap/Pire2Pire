#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>

#include <arpa/inet.h>

#include <fcntl.h>
#include <stdio.h>              // FILE
#include <string.h>             // strlen ()

#include "../../util/logger.h"  // log_failure ()
#include "../client.h"          // client_send ()
#include "../client_request.h"  // struct client_request
#include "../daemon.h"
#include "../util/cmd.h"        // struct option
#include "../util/socket.h"
#include "../file_cache.h" 
#include "../conf.h"

#define HASH_SIZE 32

extern struct daemon *daemons;
extern FILE* log_file;
extern struct file_cache *file_cache;
extern struct prefs *prefs;

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
                    "< get : seeder is %s:%d",
                    file_to_dl->seeders->ip,
                    file_to_dl->seeders->port);
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
    char *addr = file_to_dl->seeders->ip;
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
    char *readycmd = NULL;
    char *key_     = NULL;
    char *delay    = NULL;
    char *ip       = NULL;
    char *port     = NULL;
    char *protocol = NULL;
    char *beginning = NULL;
    char *end       = NULL;

    readycmd = strdup (socket_getline_with_trailer (d->socket));
    if (!readycmd)
        goto out;
    log_success (log_file, "Ok %s", readycmd);

    key_ = 1 + strchr (readycmd, ' ');

    delay = 1 + strchr (key_, ' ');
    *(strchr (key_, ' ')) = '\0';

    ip = 1 + strchr (delay, ' ');
    *(strchr (delay, ' ')) = '\0';

    port = 1 + strchr (ip, ' ');
    *(strchr (ip, ' ')) = '\0';

    protocol = 1 + strchr (port, ' ');
    *(strchr (port, ' ')) = '\0';

    beginning = 1 + strchr (protocol, ' ');
    *(strchr (protocol, ' ')) = '\0';

    end = 1 + strchr (beginning, ' ');
    *(strchr (beginning, ' ')) = '\0';


    log_success (log_file, "KEY       : %s", key_);
    log_success (log_file, "DELAY     : %s", delay);
    log_success (log_file, "IP        : %s", ip);
    log_success (log_file, "PORT      : %s", port);
    log_success (log_file, "PROTOCOL  : %s", protocol);
    log_success (log_file, "BEGINNING : %s", beginning);
    log_success (log_file, "END       : %s", end);


    /*
     * Connecting to the daemon, using the special file transfert socket.
     */
    /* TODO&FIXME: We should use all above arguments */
    int dl_sock;
    struct sockaddr_in dl_addr;
    if ((dl_sock = socket (AF_INET, SOCK_STREAM, 0)) < 0) {
        log_failure (log_file, "daemon_request_ready (): socket () failed");
        return NULL;
    }

    if (inet_pton (AF_INET, ip, &dl_addr) < 1) {
        return NULL;
    }
    
    // TODO: Verifications on port
    inet_aton (ip, &dl_addr.sin_addr);
    dl_addr.sin_port = htons (atoi (port));
    dl_addr.sin_family = AF_INET;

    
    /*
     * Getting the file
     */ 
    log_success (log_file, "Starting to get teh file");

    /*
     * TODO : Check if we actually asked for that file.
     * TODO : Check if file already exists.
     */
    /* + 2 for '/' and '\0' */
    char *full_path;
    full_path = (char *)malloc ((strlen (prefs->shared_folder)
                                + strlen (file_to_dl->filename)
                                + 2) * sizeof (char));
    sprintf (full_path, "%s/%s", 
             prefs->shared_folder, 
             file_to_dl->filename);

 
    
    // FIXME: We should not truncate the file when downloading it by blocks
    int local_file;
    //local_file = open (file_to_dl->filename, O_WRONLY | O_TRUNC);
    local_file = open (full_path, O_CREAT | O_WRONLY | O_TRUNC);
    free (full_path);
    if (local_file < 0) {
        log_failure (log_file, 
                    "daemon_request_ready (): Unable to open %s, error : %s",
                    full_path, strerror (errno));
        return NULL;
    }

    log_success (log_file, "About to connect");
    if (connect (dl_sock, (struct sockaddr *)&dl_addr, sizeof (dl_addr)) < 0) {
        log_failure (log_file, "connect failed : %s", strerror (errno));
        return NULL;
    }
    log_success (log_file, "COnnected !");
    unsigned int nb_received_sum;
    nb_received_sum = 0;



    // FIXME: nb_received_sum should be compared to end - begin
    int nb_received = 0;
    int nb_written = 0;
    #define BUFFSIZE 128
    char buffer[BUFFSIZE];
    log_success (log_file, "ABout to enter the while loop");
    while (nb_received_sum < file_to_dl->size) {
        log_success (log_file, "Receiving file...");
        nb_received = recv (dl_sock, buffer, BUFFSIZE, 0);
        if (nb_received < 0) {
            log_failure (log_file,
                        "daemon_request_ready (): recv failed");
            return NULL;
        }
        nb_received_sum += nb_received;
        while (nb_received) {
            nb_written = write (local_file, buffer, nb_received);
            if (nb_written < 0) {
                log_failure (log_file,
                            "daemon_request_ready (): write failed");
                return NULL;
            }
            nb_received -= nb_written;
        }
    }

    close (dl_sock);
    close (local_file);

out:
    return NULL;
}
