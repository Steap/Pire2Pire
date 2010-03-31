#include <arpa/inet.h>          // inet_pton ()

#include <fcntl.h>              // open ()
#include <stdio.h>              // FILE
#include <string.h>             // strcpy ()
#include <unistd.h>             // close ()

#include "../../util/logger.h"  // log_failure ()
#include "../conf.h"            // struct prefs
#include "../daemon.h"          // daemon_send ()
#include "../daemon_request.h"  // struct daemon_request
#include "../file_cache.h"      // struct file_cache
#include "../util/cmd.h"        // cmd_to_argc_argv ()

#define BUFFSIZE    128

extern struct file_cache    *file_cache;
extern sem_t                file_cache_lock;
extern struct prefs         *prefs;
extern FILE                 *log_file;

void*
daemon_request_ready (void* arg) {
    struct daemon_request   *r;
    // Parse elements
    char                    *key, *delay, *ip, *port, *proto, *begin, *end;
/* cmd version: */
    int                     argc;
    char                    **argv;
    int                     dl_sock;
    struct sockaddr_in      dl_addr;
    struct file_cache       *file;
    int                     local_file;
    int                     nb_received;
    res_size_t              nb_received_sum;
    char                    buffer[BUFFSIZE];
    int                     nb_written;
    char                    *full_path;

    r = (struct daemon_request *) arg;
    if (!r)
        return NULL;

    /*
     * cmd is supposedly:
     * ready KEY DELAY IP PORT PROTOCOL BEGINNING END
     */
    argv = cmd_to_argc_argv (r->cmd, &argc);
    if (argc != 8) {
        cmd_free (argv);
        return NULL;
    }
    key = argv[1];
    delay = argv[2];
    ip = argv[3];
    port = argv[4];
    proto = argv[5];
    begin = argv[6];
    end = argv[7];

    /* TODO&FIXME: We should use all above arguments */
    if ((dl_sock = socket (AF_INET, SOCK_STREAM, 0)) < 0) {
        log_failure (log_file, "daemon_request_ready (): socket () failed");
        return NULL;
    }

    if (inet_pton (AF_INET, ip, &dl_addr) < 1) {
        return NULL;
    }
    // TODO: Verifications on port
    dl_addr.sin_port = htons (atoi (port));

    file = file_cache_get_by_key (file_cache, key);
    if (!file)
        return NULL;

    // TODO: Check if we actually asked for that file

    // TODO: Check if file already exists

    /* + 2 for '/' and '\0' */
    full_path = (char *)malloc ((strlen (prefs->shared_folder)
                                + strlen (file->filename)
                                + 2) * sizeof (char));
    sprintf (full_path, "%s/%s", prefs->shared_folder, file->filename);
    // FIXME: We should not truncate the file when downloading it by blocks
    local_file = open (file->filename, O_WRONLY | O_TRUNC);
    free (full_path);
    if (local_file < 0) {
        log_failure (log_file, "daemon_request_ready (): Unable to open file");
        return NULL;
    }

    if (connect (dl_sock, (struct sockaddr *)&dl_addr, sizeof (dl_addr)) < 0) {
        return NULL;
    }

    nb_received_sum = 0;
    // FIXME: nb_received_sum should be compared to end - begin
    while (nb_received_sum < file->size) {
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

    cmd_free (argv);

    return NULL;
}
