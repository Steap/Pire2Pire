#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "conf.h"


#define DEFAULT_PORT              1337

#define MIN_PORT                  1024
#define MAX_PORT                 40000 // FIXME

/*
 * Default values for the daemon's prefs.
 */ 
struct prefs p= {
    .port = DEFAULT_PORT
};

struct prefs *conf_retrieve (const char *path) {
    FILE           *conf_file;
    struct prefs   *prefs;
    char           key[256], value[256];

    prefs = &p;

    if ((conf_file = fopen (path, "r")) == NULL) {
        fprintf (stderr, "Cant open conf\n");
        return prefs;
    }

    while (fscanf (conf_file, "%s = %s\n", key, value) != EOF) {
        if (strcmp (key, "port") == 0) {
            int port = atoi (value);
            if (port > MIN_PORT)
                prefs->port = atoi (value);
        }
    }
    
    fclose (conf_file);
    return prefs;
}
