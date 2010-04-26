#ifndef CONF_H
#define CONF_H

struct prefs {
    int         client_port;
    int         daemon_port;
    char        *shared_folder;
    char        *interface;
    int         nb_proc;
    int         max_clients;
    int         max_requests_per_client;
    int         max_daemons;
};

struct prefs *conf_retrieve (const char*);
void          conf_free (struct prefs *);

#endif
