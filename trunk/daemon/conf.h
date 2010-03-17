#ifndef CONF_H
#define CONF_H

struct prefs {
    int         client_port;
    int         daemon_port;
    char        *shared_folder;
};

struct prefs *conf_retrieve (const char*);
void          conf_free (struct prefs *);

#endif
