#ifndef CONF_H
#define CONF_H

struct prefs {
    int         client_port;
    int         daemon_port;
};

struct prefs *conf_retrieve (const char*);

#endif
