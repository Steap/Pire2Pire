#include <stdio.h>  // FILE
#include <stdlib.h> // atoi ()
#include <string.h> // strcmp ()

#include "conf.h"   // struct prefs

#define DEFAULT_CLIENT_PORT     1337
#define DEFAULT_DAEMON_PORT     7331

#define MIN_PORT                  1024
#define MAX_PORT                 40000 // FIXME

/*
 * Default values for the daemon's prefs.
 */ 
struct prefs p = {
    .client_port = DEFAULT_CLIENT_PORT,
    .daemon_port = DEFAULT_DAEMON_PORT
};

struct prefs *conf_retrieve (const char *path) {
    FILE            *conf_file;
    struct prefs    *prefs;
    char            key[256], value[256];
    int             port;

    prefs = &p;

    if ((conf_file = fopen (path, "r")) == NULL) {
        fprintf (stderr, "Cant open conf\n");
        return prefs;
    }

    while (fscanf (conf_file, "%s = %s\n", key, value) != EOF) {
        if (strcmp (key, "client_port") == 0) {
            port = atoi (value);
            if (port > MIN_PORT)
                prefs->client_port = port;
        }
        else if (strcmp (key, "daemon_port") == 0) {
            port = atoi (value);
            if (port > MIN_PORT)
                prefs->daemon_port = port;
        }
    }
    
    fclose (conf_file);
    return prefs;
}
