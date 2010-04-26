#ifndef DAEMON_REQUEST_H
#define DAEMON_REQUEST_H

#include "daemon.h"
#include "thread_pool.h"

struct daemon;

struct daemon_request {
    char                    *cmd;
    struct daemon           *daemon;
    struct pool             *pool;
    void *                  (*handler) (void *);
    struct daemon_request   *prev;
    struct daemon_request   *next;
};

struct daemon_request *
daemon_request_new  (char *cmd,
                    struct daemon *daemon,
                    struct pool *pool,
                    void * (*func) (void *));

void
daemon_request_free (struct daemon_request *r);

struct daemon_request *
daemon_request_add  (struct daemon_request *l,
                    struct daemon_request *r);

struct daemon_request *
daemon_request_remove   (struct daemon_request *l,
                        struct daemon_request *r);

int
daemon_request_count (struct daemon_request *l);

void *
daemon_request_handler (void *arg);

#endif//DAEMON_REQUEST_H
