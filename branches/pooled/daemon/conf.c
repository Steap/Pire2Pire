#include <stdio.h>  // FILE
#include <stdlib.h> // atoi ()
#include <string.h> // strcmp ()

#include "conf.h"   // struct prefs

/*
 * Default values for the daemon's prefs.
 */
#define DEFAULT_CLIENT_PORT         1337
#define DEFAULT_DAEMON_PORT         7331
#define DEFAULT_SHARED_FOLDER       "/tmp"
#define DEFAULT_INTERFACE           "eth0"
#define DEFAULT_NB_PROC             2
#define DEFAULT_MAX_CLIENTS         3
#define DEFAULT_MAX_RQ_PER_CLIENT   10
#define DEFAULT_MAX_DAEMONS         10
#define DEFAULT_MAX_RQ_PER_DAEMON   10

 /*
 * Valid values for our ports.
 * See http://www.iana.org/assignments/port-numbers
 */
#define MIN_PORT                  1024
#define MAX_PORT                 49151

struct prefs*
conf_retrieve (const char *path) {
    FILE            *conf_file;
    struct prefs    *prefs;
    char            key[256], value[256];
    int             num_val;

    prefs = malloc (sizeof (struct prefs));
    if (!prefs)
        return NULL;

    prefs->client_port      = DEFAULT_CLIENT_PORT;
    prefs->daemon_port      = DEFAULT_DAEMON_PORT;
    prefs->shared_folder    = DEFAULT_SHARED_FOLDER;
    prefs->interface        = DEFAULT_INTERFACE;
    prefs->nb_proc          = DEFAULT_NB_PROC;
    prefs->max_clients      = DEFAULT_MAX_CLIENTS;
    prefs->max_daemons      = DEFAULT_MAX_DAEMONS;
    prefs->max_requests_per_client = DEFAULT_MAX_RQ_PER_CLIENT;
    prefs->max_requests_per_daemon = DEFAULT_MAX_RQ_PER_DAEMON;

    if ((conf_file = fopen (path, "r")) == NULL) {
        fprintf (stderr, "Cant open conf\n");
        return prefs;
    }

    while (fscanf (conf_file, "%s = %s\n", key, value) != EOF) {
        if (strcmp (key, "client_port") == 0) {
            num_val = atoi (value);
            if (num_val > MIN_PORT)
                prefs->client_port = num_val;
        }
        else if (strcmp (key, "daemon_port") == 0) {
            num_val = atoi (value);
            if (num_val > MIN_PORT)
                prefs->daemon_port = num_val;
        }
        else if (strcmp (key, "shared_folder") == 0) {
            prefs->shared_folder = strdup (value);
        }
        else if (strcmp (key, "interface") == 0) {
            prefs->interface = strdup (value);
        }
        else if (strcmp (key, "nb_proc") == 0) {
            num_val = atoi (value);
            if (num_val > 1)
                prefs->nb_proc = atoi (value);
        }
        else if (strcmp (key, "max_clients") == 0) {
            num_val = atoi (value);
            if (num_val > 1)
                prefs->max_clients = atoi (value);
        }
        else if (strcmp (key, "max_daemons") == 0) {
            num_val = atoi (value);
            if (num_val > 1)
                prefs->max_daemons = atoi (value);
        }
        else if (strcmp (key, "max_requests_per_client") == 0) {
            num_val = atoi (value);
            if (num_val > 1)
                prefs->max_requests_per_client = atoi (value);
        }
        else if (strcmp (key, "max_requests_per_daemon") == 0) {
            num_val = atoi (value);
            if (num_val > 1)
                prefs->max_requests_per_daemon = atoi (value);
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
