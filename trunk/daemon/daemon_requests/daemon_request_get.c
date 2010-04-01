#include <sys/stat.h>           // entry_stat

#include <arpa/inet.h>          // inet_pton ()
#include <netinet/in.h>         // struct sockaddr_in

#include <dirent.h>             // DIR
#include <errno.h>
#include <fcntl.h>              // open ()
#include <stdlib.h>             // malloc ()
#include <string.h>             // strcmp ()
#include <unistd.h>             // close ()

#include "../../util/md5/md5.h" // MDFile ()
#include "../../util/logger.h"  // log_failure ()
#include "../conf.h"            // struct prefs
#include "../daemon.h"          // daemon_send ()
#include "../daemon_request.h"  // struct daemon_request
#include "../file_cache.h"      // file_size_t
#include "../util/cmd.h"        // cmd_to_argc_argv ()

#define BUFFSIZE    128

extern struct prefs *prefs;
extern FILE         *log_file;
extern char         my_ip[INET_ADDRSTRLEN];

static char*
get_md5 (const char *path) {
    int           i;
    unsigned char digest[16];
    char          *hash;
    
    hash = malloc (33);
    MDFile (&digest, path);
    if (!hash)
        return NULL;
    hash[32] = '\0';
    for (i = 0; i < 16; i++) {
        sprintf (hash+2*i, "%02x", digest[i]);
    }
    return hash;
}

void*
daemon_request_get (void *arg) {
    struct daemon_request   *r;
    char                    answer[512];
    DIR                     *dir;
    struct dirent           *entry;
    char                    full_path[512];
    struct stat             entry_stat;
    char                    *key;
    int                     file;
/* cmd version: */
    int                     argc;
    char                    **argv;
    int                     listen_sock;
    struct sockaddr_in      listen_addr;
    socklen_t               listen_addr_size;
    int                     data_sock;
    file_size_t              to_be_sent;
    char                    buffer[BUFFSIZE];
    int nb_read;
    int nb_sent;

    r = (struct daemon_request *) arg;
    if (!r)
        return NULL;


    /*
     * cmd is supposedly:
     * get KEY BEGINNING END
     */
    argv = cmd_to_argc_argv (r->cmd, &argc);
    if (argc != 4) {
        cmd_free (argv);
        // TODO: send an error message everytime we return like this...
        return NULL;
    }

    r = (struct daemon_request *) arg;
    if (!r)
        return NULL;

    dir = opendir (prefs->shared_folder);
    if (dir == NULL) {
        log_failure (log_file,
                    "daemon_request_list () : Unable to opendir %s",
                    prefs->shared_folder);
        return NULL;
    }
    // Browsing my own files
    for (entry = readdir (dir); entry != NULL; entry = readdir (dir)) {
        // Listing all regular files
        if (entry->d_type == DT_REG) {
            // FIXME: buffer overflow
            sprintf (full_path, "%s/%s", prefs->shared_folder, entry->d_name);
            if (stat (full_path, &entry_stat) < 0) {
                log_failure (log_file,
                            "daemon_request_get (): can't stat file %s",
                            full_path);
                continue;
            }
            key = get_md5 (full_path);
            /* TODO: Free md5 */
            if (strcmp (key, argv[1]) == 0)
                break;
        }
    }
    if (!entry) {
        // TODO: send an error message
        log_failure (log_file,
                    "daemon_request_get (): Unable to locate file with key %s",
                    argv[1]);
        return NULL;
    }

    if (closedir (dir) < 0) {
        log_failure (log_file,
                    "daemon_request_get (): can't close shared directory");
        return NULL;
    }

    file = open (full_path, O_RDONLY);
    if (file < 0) {
         log_failure (log_file,
                    "daemon_request_get (): can't open file %s",
                    full_path);
        return NULL;

    }

    if ((listen_sock = socket (AF_INET, SOCK_STREAM, 0)) < 0) {
        log_failure (log_file, "daemon_request_get (): socket () failed");
        return NULL;
    }

    listen_addr.sin_family = AF_INET;
/* FIXME: this is not working, I don't know why
    if (inet_pton (AF_INET, r->daemon->addr, &listen_addr) < 1) {
        log_failure (log_file, "daemon_request_get (): inet_pton () failed");
        return NULL;
    }
*/
    listen_addr.sin_addr.s_addr = htonl (INADDR_ANY);

    /* Let the OS give you a port */
    listen_addr.sin_port = 0;

    listen_addr_size = sizeof (listen_addr);

    if (bind (listen_sock,
                (struct sockaddr *)&listen_addr,
                listen_addr_size) < 0) {
        log_failure (log_file, "daemon_request_get (): bind () failed %d", errno);
        return NULL;
    }

    if (getsockname (listen_sock,
                        (struct sockaddr *)&listen_addr,
                        &listen_addr_size) < 0) {
        log_failure (log_file, "daemon_request_get (): getsockname () failed");
    }

    // FIXME: beginning, end, protocol and delay are unused
    sprintf (answer,
                "ready %s 0 %s %d tcp 0 0\n",
                key,
                my_ip,
                ntohs (listen_addr.sin_port));

    if (listen (listen_sock, 1) < 0) {
        log_failure (log_file, "daemon_request_get (): listen () failed");
        return NULL;
    }

    daemon_send (r->daemon, answer);

    data_sock = accept (listen_sock, NULL, 0);
    if (data_sock < 0) {
        log_failure (log_file, "daemon_request_get (): accept () failed");
        return NULL;
    }

    close (listen_sock);

    to_be_sent = entry_stat.st_size;
    while (to_be_sent) {
        nb_read = read (file, buffer, BUFFSIZE);
        while (nb_read) {
            nb_sent = send (data_sock, buffer, nb_read, 0);
            to_be_sent -= nb_sent;
            nb_read -= nb_sent;
        }
    }

    close (data_sock);

    close (file);

    return NULL;
}

