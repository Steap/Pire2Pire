#include <stdio.h>  // FILE
#include <stdlib.h> // atoi ()
#include <string.h> // strcmp ()

#include "conf.h"   // struct prefs

/*
 * Default values for the daemon's prefs.
 */ 
#define DEFAULT_CLIENT_PORT      1337
#define DEFAULT_DAEMON_PORT      7331
#define DEFAULT_SHARED_FOLDER   "/tmp"
#define DEFAULT_INTERFACE       "eth0"

#define MIN_PORT                  1024
#define MAX_PORT                 40000 // FIXME

struct prefs*
conf_retrieve (const char *path) {
    FILE            *conf_file;
    struct prefs    *prefs;
    char            key[256], value[256];
    int             port;

    prefs = malloc (sizeof (struct prefs));
    if (!prefs)
        return NULL;

    prefs->client_port   = DEFAULT_CLIENT_PORT; 
    prefs->daemon_port   = DEFAULT_DAEMON_PORT;
    prefs->shared_folder = DEFAULT_SHARED_FOLDER;
    prefs->interface     = DEFAULT_INTERFACE;

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
        else if (strcmp (key, "shared_folder") == 0) {
            prefs->shared_folder = strdup (value);
        }
        else if (strcmp (key, "interface") == 0) {
            prefs->interface = strdup (value);
        }
    }
    
    fclose (conf_file);
    return prefs;
}

void
conf_free (struct prefs *p) {
    if (!p)
        return;
    
    if (p->shared_folder)
        free (p->shared_folder);
    if (p->interface)
        free (p->interface);

    free (p);
}
