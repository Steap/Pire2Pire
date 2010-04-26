#ifndef CLIENT_REQUEST_H
#define CLIENT_REQUEST_H

#include "client.h"
#include "thread_pool.h"

struct client_request {
    char                    *cmd;
    struct client           *client;
    void *                  (*handler) (void *);
    struct pool             *pool;
    struct client_request   *prev;
    struct client_request   *next;
};

struct client_request *
client_request_new (char *cmd,
                    struct client *client,
                    struct pool *pool,
                    void * (*func) (void *));

void
client_request_free (struct client_request *r);

struct client_request *
client_request_add (struct client_request *l,
                    struct client_request *r);

struct client_request *
client_request_remove (struct client_request *l,
                        struct client_request *r);

int
client_request_count (struct client_request *l);

void * client_request_handler (void *arg);

#endif//CLIENT_REQUEST_H
