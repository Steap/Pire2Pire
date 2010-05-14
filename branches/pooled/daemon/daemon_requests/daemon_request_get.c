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
#include "../util/cmd_parser.h" // cmd_parse ()

#define BUFFSIZE    128

extern struct prefs *prefs;
extern FILE         *log_file;
extern char         my_ip[INET_ADDRSTRLEN];

static struct daemon_request    *r;
static struct sockaddr_in       listen_addr;
/* Full path to the requested file */
static char                     full_path[512];
/* The socket we will wait for the peer to connect on */
static int                      listen_sock;
static struct dirent            *entry;         /* The file we will send */
static struct stat              entry_stat;     /* Information about the file */
/* File descriptor of the requested file */
static int                      file;
static char                     *key;           /* MD5 hash of the file */
static struct parsed_cmd        *pcmd;          /* Parsed command */
/* The connected socket we will actually use to send the file */
static int                  data_sock;

static int find_file ();
static int prepare_sending ();
static int send_file ();

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
    char                    answer[512];
    struct option_template options [] = {
        {0, NULL, NO_ARG}
    };
    char                    error_buffer[BUFFSIZE];

    listen_sock = -1;
    data_sock = -1;
    file = -1;

    r = (struct daemon_request *) arg;
    if (!r)
        return NULL;

    /*
     * cmd is supposedly:
     * get KEY BEGINNING END
     */
    if (cmd_parse_failed (pcmd = cmd_parse (r->cmd, options))) {
        sprintf (error_buffer,
                 "error %s: Command parse failed",
                 __FUNCTION__);
        daemon_send (r->daemon, error_buffer);
        return NULL;
    }
    if (pcmd->argc != 4) {
        cmd_parse_free (pcmd);
        sprintf (error_buffer,
                 "error %s: Invalid number of arguments",
                 __FUNCTION__);
        daemon_send (r->daemon, error_buffer);
        return NULL;
    }

    /*
     * First we find the file by its key, and store its full path in full_path
     */
    if (find_file () < 0)
        goto out;

    file = open (full_path, O_RDONLY);
    if (file < 0) {
         log_failure (log_file,
                        "dr_get (): open () failed for file %s",
                        full_path);
        goto out;
    }

    /*
     * We prepare a socket to send the file
     */
    if (prepare_sending () < 0)
        goto out;

    /*
     * We notify the daemon that the file is ready
     */
    /* FIXME: beginning, end, protocol and delay are unused */
    sprintf (answer,
                "ready %s 0 %s %d tcp %s %s\n",
                key,
                my_ip,
                ntohs (listen_addr.sin_port),
                pcmd->argv[2],  /* BEGINNING */
                pcmd->argv[3]); /* END */
    daemon_send (r->daemon, answer);

    /*
     * Then we wait for the daemon to connect
     */
    data_sock = accept (listen_sock, NULL, 0);
    if (data_sock < 0) {
        log_failure (log_file, "dr_get (): accept () failed");
        goto out;
    }

    /*
     * FIXME: If the guy that connected isn't the daemon, we should stop
     * and accept again
     */
    close (listen_sock);

    /*
     * And send the file
     */
    if (send_file () < 0)
        goto out;

    return NULL;

out:
    if (listen_sock >= 0)
        close (listen_sock);
    if (data_sock >= 0)
        close (data_sock);
    if (file >= 0)
        close (file);

    return NULL;
}

static int
find_file () {
    DIR                     *dir;

    dir = opendir (prefs->shared_folder);
    if (dir == NULL) {
        log_failure (log_file,
                    "dr_get () : opendir () failed for dir %s",
                    prefs->shared_folder);
        return -1;
    }
    /* Browsing my own files */
    for (entry = readdir (dir); entry != NULL; entry = readdir (dir)) {
        /* Listing all regular files */
        if (entry->d_type == DT_REG) {
            /* FIXME: buffer overflow */
            sprintf (full_path, "%s/%s", prefs->shared_folder, entry->d_name);
            if (stat (full_path, &entry_stat) < 0) {
                log_failure (log_file,
                            "dr_get (): stat () failed for file %s",
                            full_path);
                continue;
            }
            key = get_md5 (full_path);
            /* TODO: Free md5 */
            if (strcmp (key, pcmd->argv[1]) == 0)
                break;
            free (key);
        }
    }
    if (!entry) {
        log_failure (log_file,
                        "dr_get (): Unable to locate file with key %s",
                        pcmd->argv[1]);
        return -1;
    }

    if (closedir (dir) < 0) {
        log_failure (log_file, "dr_get (): closedir () failed");
        return -1;
    }

    return 0;
}

static int
prepare_sending () {
    socklen_t               listen_addr_size;

    if ((listen_sock = socket (AF_INET, SOCK_STREAM, 0)) < 0) {
        log_failure (log_file, "dr_get: socket () failed");
        return -1;
    }

    listen_addr.sin_family = AF_INET;
    listen_addr.sin_addr.s_addr = htonl (INADDR_ANY);
    listen_addr.sin_port = 0;           /* The OS finds an available port */
    listen_addr_size = sizeof (listen_addr);

    if (bind (listen_sock,
                (struct sockaddr *)&listen_addr,
                listen_addr_size) < 0) {
        log_failure (log_file,
                    "dr_get: bind () failed - %s",
                    strerror (errno));
        return -1;
    }

    if (getsockname (listen_sock,
                        (struct sockaddr *)&listen_addr,
                        &listen_addr_size) < 0) {
        log_failure (log_file, "dr_get (): getsockname () failed");
    }

    if (listen (listen_sock, 1) < 0) {
        log_failure (log_file, "dr_get (): listen () failed");
        return -1;
    }

    return 0;
}

static int
send_file () {
    int             nb_read;
    int             nb_sent;
    file_size_t     to_be_sent;
    char            buffer[BUFFSIZE];

    to_be_sent = entry_stat.st_size;
    while (to_be_sent) {
        nb_read = read (file, buffer, BUFFSIZE);
        if (nb_read < 0) {
            log_failure (log_file,
                        "dr_get (): read failed in send_file ()");
            return -1;
        }
        while (nb_read) {
            nb_sent = send (data_sock, buffer, nb_read, 0);
            if (nb_sent < 0) {
                log_failure (log_file,
                            "dr_get (): send failed in send_file ()");
                return -1;
            }
            to_be_sent -= nb_sent;
            nb_read -= nb_sent;
        }
    }

    close (data_sock);
    close (file);

    return 0;
}
