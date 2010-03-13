#ifndef CLIENT_REQUEST_H
#define CLIENT_REQUEST_H

struct client;

struct client_request {
    char *cmd;
    struct client *client;
    struct client_request *prev;
    struct client_request *next;
    pthread_t thread_id;
};

struct client_request *
client_request_new (char *cmd, struct client *client);

void
client_request_free (struct client_request *r);

struct client_request *
client_request_add (struct client_request *l, struct client_request *r);

struct client_request *
client_request_remove (struct client_request *l, pthread_t pt);

int
client_request_count (struct client_request *l);

#endif//CLIENT_REQUEST_H
