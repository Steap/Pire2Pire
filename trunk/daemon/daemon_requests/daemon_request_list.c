#include <sys/stat.h>           // entry_stat

#include <arpa/inet.h>          // inet_ntop ()
#include <netinet/in.h>         // struct sockaddr_in

#include <stdio.h>              // NULL
#include <stdlib.h>             // malloc ()
#include <dirent.h>             // DIR

#include "../../util/md5/md5.h" // MDFile ()
#include "../../util/logger.h"  // log_failure ()
#include "../daemon.h"          // daemon_send ()
#include "../daemon_request.h"  // struct daemon_request

#include "../conf.h"
extern struct prefs *prefs;
//#define SHARED_FOLDER "/tmp/lol/"
#define SHARED_FOLDER prefs->shared_folder

extern FILE *log_file;
extern struct daemon *daemons;

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
daemon_request_list (void *arg) {
    struct daemon_request   *r;
    char                    answer[512];
    DIR                     *dir;
    struct dirent           *entry;
    char                    entry_full_path[256];
    struct stat             entry_stat;
    char                    *key;
    struct sockaddr_in      my_addr;
    socklen_t               my_addr_size;
    // FIXME: We should define an IP(v4,v6)_MAXSIZE somewhere
    #define IP_MAXSIZE      15
    char                    my_ip[IP_MAXSIZE + 1];

    /* OKAY, let's say all options/args are silently ignored */

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

    // We must retrieve our own address from the socket
    // Not sure we have to lock but it seems a good idea
    sem_wait (&r->daemon->socket_lock);
    getsockname (r->daemon->socket,
                    (struct sockaddr *)&my_addr,
                    &my_addr_size);
    sem_post (&r->daemon->socket_lock);
    // Trying conversion to a string
    if (!inet_ntop (AF_INET, &my_addr.sin_addr, my_ip, IP_MAXSIZE + 1))
        return NULL;

    for (entry = readdir (dir); entry != NULL; entry = readdir (dir)) {
        if (entry->d_type == DT_REG) {
            sprintf (entry_full_path,
                    "%s/%s",
                    prefs->shared_folder,
                    entry->d_name);
            if (stat (entry_full_path, &entry_stat) < 0) {
                log_failure (log_file,
                            "daemon_request_list () : can't stat file %s",
                            entry_full_path);
                continue;
            }

            key = get_md5 (entry_full_path);
            if (!key)
                continue;



            sprintf (answer, "file %s %s %d %s:%d\n",
                    entry->d_name,
                    key,
                    (int) entry_stat.st_size,
                    my_ip,
                    7331); //FIXME: Our port is in prefs somewhere

            if (daemon_send (r->daemon, answer) < 0) {
                log_failure (log_file,
                            "do_list () : failed to send data to daemon");
            }

            free (key);
            key = NULL;
        }
    }

    if (closedir (dir) < 0) {
        log_failure (log_file, "do_list () : can't close shared directory");
        return NULL;
    }

    return NULL; 
}

