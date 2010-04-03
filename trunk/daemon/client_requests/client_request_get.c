#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>

#include <arpa/inet.h>

#include <errno.h>
#include <fcntl.h>
#include <semaphore.h>
#include <stdio.h>              // FILE
#include <string.h>             // strlen ()

#include "../../util/logger.h"  // log_failure ()
#include "../client.h"          // client_send ()
#include "../client_request.h"  // struct client_request
#include "../daemon.h"
#include "../util/cmd_parser.h" // struct option_template
#include "../util/socket.h"
#include "../file_cache.h"
#include "../conf.h"

#define RECV_BUFFSIZE   128

extern struct daemon        *daemons;
extern sem_t                daemons_lock;
extern FILE*                log_file;
extern struct file_cache    *file_cache;
extern sem_t                file_cache_lock;
extern struct prefs         *prefs;

static const struct client_request  *r;
static char                         answer[256];
/* Will point the file cache entry of the file we will download */
static const struct file_cache      *file_to_dl;
/* Daemon who should send us the file */
static struct daemon                *d;
/* File descriptor of the file being downloaded */
static int                          local_file;
/* The socket we will actually use for download */
static int                          dl_sock;
/* The ready command parsed */
struct parsed_cmd                   *parsed_ready;

static int find_file ();
static int find_daemon ();
static int prepare_file ();
static int connect_to_daemon ();
static int download_file ();

#include <unistd.h>
void*
client_request_get (void *arg) {
    char *readycmd;

    dl_sock = -1;
    local_file = -1;

    r = (struct client_request *) arg;
    if (!r)
        return NULL;


    /* First we find the file in the cache */
    if (find_file () < 0)
        goto out;
    /* file_to_dl should now be the good one */


    /* Find the daemon owning the file by checking its IP */
    if (find_daemon () < 0)
        goto out;
    /* d should now point to the good daemon */


    /* Sending the "get key begin end" message */
    sprintf (answer,
             "get %s %d %ld\n",
             file_to_dl->key,
             0,
             file_to_dl->size);
    if (daemon_send (d, answer) < 0) {
        log_failure (log_file, "cr_get: could not send the ready command");
        goto out;
    }


    /*
     * Reading the answer ("ready")
     * TODO : what if it never comes ? What if it is not "ready" ?
     * Cmd is supposedly :
     * ready KEY DELAY IP PORT PROTOCOL BEGINNING END
     *   0    1    2   3   4       5        6      7
     */
    readycmd = socket_getline_with_trailer (d->socket);
    if (!readycmd)
        goto out;
    if (cmd_parse_failed ((parsed_ready = cmd_parse (readycmd, NULL)))) {
        free (readycmd);
        goto out;
    }
    free (readycmd);
    if (parsed_ready->argc < 8) {
        goto out;
    }


    /* Prepare the file for download */
    if (prepare_file () < 0)
        goto out;


    /* Connect to the peer */
    if (connect_to_daemon () < 0)
        goto out;


    if (download_file () < 0)
        goto out;


    return NULL;



out:
    if (local_file >= 0)
        close (local_file);
    if (dl_sock >= 0)
        close (dl_sock);
    if (parsed_ready)
        cmd_parse_free (parsed_ready);
    return NULL;
}

static int find_file () {
    struct parsed_cmd   *parsed_get;

    if (cmd_parse_failed ((parsed_get = cmd_parse (r->cmd, NULL))))
        return -1;

    /*
     *  Checking whether the arguments are valid.  If they are, retrieves the
     *  data (stored in the cache) about the file to be downloaded
     */
    if (parsed_get->argc == 1) {
        if (client_send (r->client,
                            " < get: you need to specify a key\n") < 0)
            log_failure (log_file, "cr_get: client_send () failed");
        goto error;
    }
    else {
        if (strlen (parsed_get->argv[1]) != FILE_KEY_SIZE) {
            if (client_send (r->client,
                                " < get: hash size is not good, man\n") < 0)
                log_failure (log_file, "cr_get: client_send () failed");
            goto error;
        }
        else {
            sem_wait (&file_cache_lock);
            file_to_dl = file_cache_get_by_key (file_cache, parsed_get->argv[1]);
            sem_post (&file_cache_lock);
            if (!file_to_dl) {
                if (client_send (r->client,
                                " < get: key not in cache, please list\n") < 0)
                    log_failure (log_file, "cr_get: client_send () failed");
                goto error;
            }
            sprintf (answer,
                    "< get : seeder is %s:%d",
                    file_to_dl->seeders->ip,
                    file_to_dl->seeders->port);
            if (client_send (r->client, answer) < 0) {
                log_failure (log_file, "cr_get: client_send () failed");
                goto error;
            }
        }
    }
    cmd_parse_free (parsed_get);

    if (client_send (r->client, answer) < 0) {
        log_failure (log_file, "cr_get: client_send () failed");
        return -1;
    }

    return 0;

error:
    if (parsed_get)
        cmd_parse_free (parsed_get);
    return -1;
}

static int
find_daemon () {
    char                    *addr;

    addr = file_to_dl->seeders->ip;
    sem_wait (&daemons_lock);
    /*
     * Retrieving the daemon
     * FIXME : use a function in daemon.c
     * Assigned to : roelandt
     */
    for_each_daemon (d) {
        if (strcmp (d->addr, addr) == 0)
            break;
    }
    sem_post (&daemons_lock);
    if (!d)
        return -1;

    return 0;
}

static int
prepare_file () {
    char *full_path;

    /* TODO : Check if we actually asked for that file */
    /* TODO : Check if file already exists */
    /* + 2 for '/' and '\0' */
    full_path = (char *)malloc ((strlen (prefs->shared_folder)
                                + strlen (file_to_dl->filename)
                                + 2) * sizeof (char));

    sprintf (full_path, "%s/%s", prefs->shared_folder, file_to_dl->filename);

    /* FIXME: We should not truncate the file when downloading it by blocks */
    local_file = open (full_path, O_CREAT | O_WRONLY | O_TRUNC, (mode_t)0644);
    if (local_file < 0) {
        log_failure (log_file,
                    "cr_get: Unable to open %s, error : %s",
                    full_path,
                    strerror (errno));
        free (full_path);
        return -1;
    }
    free (full_path);

    /*
     * TODO: We should mmap the file, then close the file descriptor and use
     * the map, then munmap it at the end
     */

    return 0;
}

static int
connect_to_daemon () {
    struct sockaddr_in      dl_addr;

    /* TODO: Use the protocol specified */
    if ((dl_sock = socket (AF_INET, SOCK_STREAM, 0)) < 0) {
        log_failure (log_file, "cr_get (): socket () failed");
        return -1;
    }

    dl_addr.sin_family = AF_INET;
    dl_addr.sin_port = htons (atoi (parsed_ready->argv[4]));
    // TODO: Verifications on port
    if (inet_pton (AF_INET, parsed_ready->argv[3], &dl_addr.sin_addr) < 1) {
        return -1;
    }

    if (connect (dl_sock, (struct sockaddr *)&dl_addr, sizeof (dl_addr)) < 0) {
        log_failure (log_file,
                    "cr_get: connect failed, error : %s",
                    strerror (errno));
        return -1;
    }

    return 0;
}

static int
download_file () {

    unsigned int    nb_received_sum;
    int             nb_received;
    int             nb_written;
    char            buffer[RECV_BUFFSIZE];

    /* TODO: We should use a mmapped region instead of write () */

    nb_received_sum = 0;
    /* FIXME: nb_received_sum should be compared to end - begin */
    while (nb_received_sum < file_to_dl->size) {
        nb_received = recv (dl_sock, buffer, RECV_BUFFSIZE, 0);
        if (nb_received < 0) {
            log_failure (log_file, "cr_get: recv () failed");
            return -1;
        }
        nb_received_sum += nb_received;
        while (nb_received) {
            nb_written = write (local_file, buffer, nb_received);
            if (nb_written < 0) {
                log_failure (log_file, "cr_get: write () failed");
                return -1;
            }
            nb_received -= nb_written;
        }
    }

    if (close (dl_sock) < 0) {
        log_failure (log_file, "cr_get: close () failed");
        return -1;
    }
    dl_sock = -1;

    /* TODO: With mmap, local_file should already be closed */
    if (close (local_file) < 0) {
        log_failure (log_file, "cr_get: close () failed");
        return -1;
    }
    local_file = -1;

    return 0;
}
