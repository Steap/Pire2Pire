#include <sys/socket.h>

#include <arpa/inet.h>          // inet_pton ()

#include <errno.h>              // errno
#include <fcntl.h>              // open ()
#include <stdio.h>              // FILE
#include <string.h>             // strcpy ()
#include <unistd.h>             // close ()

#include "../util/logger.h"  // log_failure ()
#include "../conf.h"            // struct prefs
#include "../daemon.h"          // daemon_send ()
#include "../daemon_request.h"  // struct daemon_request
#include "../dl_file.h"
#include "../file_cache.h"      // struct file_cache
#include "../util/cmd.h"        // cmd_to_argc_argv ()

#define BUFFSIZE    128

extern struct file_cache    *file_cache;
extern sem_t                file_cache_lock;
extern struct prefs         *prefs;
extern FILE                 *log_file;
extern sem_t                downloads_lock;
extern struct dl_file       *downloads;

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
    file_size_t             nb_received_sum;
    char                    buffer[BUFFSIZE];
    int                     nb_written;
    char                    *full_path;
    char                    error_buffer[BUFFSIZE];

    r = (struct daemon_request *) arg;
    if (!r)
        return NULL;

    /*
     * cmd is supposedly:
     * ready KEY DELAY IP PORT PROTOCOL BEGINNING END
     */
    argv = cmd_to_argc_argv (r->cmd, &argc);
    if (argc < 8) {
        cmd_free (argv);
        sprintf (error_buffer, 
                 "error %s: Invalid number of arguments",
                 __FUNCTION__);
        daemon_send (r->daemon, error_buffer);
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

    dl_addr.sin_family = AF_INET;
    if (inet_pton (AF_INET, ip, &dl_addr.sin_addr) < 1) {
        log_failure (log_file, "dr_ready: inet_pton () failed");
        return NULL;
    }
    // TODO: Verifications on port
    dl_addr.sin_port = htons (atoi (port));

    file = file_cache_get_by_key (file_cache, key);
    if (!file) {
        log_failure (log_file, "dr_ready: file_cache_get_by_key () failed");
        return NULL;
    }

    // TODO: Check if we actually asked for that file

    // TODO: Check if file already exists

    /* + 2 for '/' and '\0' */
    full_path = (char *)malloc ((strlen (prefs->shared_folder)
                                + strlen (file->filename)
                                + 2) * sizeof (char));
    sprintf (full_path, "%s/%s", prefs->shared_folder, file->filename);
    // FIXME: We should not truncate the file when downloading it by blocks
    local_file = open (full_path,
                        O_WRONLY | O_TRUNC | O_CREAT,
                        (mode_t)0644);
    //free (full_path);
    if (local_file < 0) {
        log_failure (log_file,
                    "dr_ready: open () failed, error: %s",
                    strerror (errno));
        return NULL;
    }

    if (connect (dl_sock, (struct sockaddr *)&dl_addr, sizeof (dl_addr)) < 0) {
        log_failure (log_file,
                    "dr_ready: connect () failed, error: %s",
                    strerror (errno));
        return NULL;
    }

    /*
     * Downloading the file
     */

    /* Let's upload the download queue */
    struct dl_file *f;
    
    f = dl_file_new (full_path, file->size);
    if (!f) {
        log_failure (log_file, "struct dl_file is NULL :(");
        goto out;
    }

    sem_wait (&downloads_lock);
    downloads = dl_file_add (downloads, f);
    if (!downloads) {
        log_failure (log_file, 
                    "Could not add the file to the download queue\n");
        goto out;
    }
    sem_post (&downloads_lock);

    nb_received_sum = 0;
    // FIXME: nb_received_sum should be compared to end - begin
    sleep (2);
    while (nb_received_sum < file->size) {
        log_failure (log_file,
                     "DBG %d %d",
                     nb_received_sum,
                     file->size);
                     
        nb_received = recv (dl_sock, buffer, BUFFSIZE, 0);
        if (nb_received < 0) {
            log_failure (log_file,
                        "dr_ready: recv () failed");
            return NULL;
        }
        nb_received_sum += nb_received;
        while (nb_received) {
            nb_written = write (local_file, buffer, nb_received);
            if (nb_written < 0) {
                log_failure (log_file,
                            "dr_ready: write () failed");
                return NULL;
            }
            nb_received -= nb_written;
        }
    }

    /* 
     * Releasing the file from the download queue 
     */
    sem_wait (&downloads_lock);
    downloads = dl_file_remove (downloads, f);
    dl_file_free (f);
    sem_post (&downloads_lock);

    log_success (log_file, 
                "dr_ready: Received block completely %s",
                full_path);

    close (dl_sock);
    close (local_file);


out:
    if (full_path)
        free (full_path);
    if (argv)
        cmd_free (argv);
    return NULL;
}
