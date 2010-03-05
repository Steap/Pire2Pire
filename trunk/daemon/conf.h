#ifndef CONF_H
#define CONF_H

struct prefs {
    int        port;
};

struct prefs *conf_retrieve (const char*);

#endif
