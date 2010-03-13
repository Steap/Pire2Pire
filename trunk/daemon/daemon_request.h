#ifndef DAEMON_REQUEST_H
#define DAEMON_REQUEST_H

struct daemon;

struct daemon_request {
    char                    *cmd;
    struct daemon           *daemon;
    struct daemon_request   *prev;
    struct daemon_request   *next;
    pthread_t               thread_id;
};

struct daemon_request
*daemon_request_new (char *cmd, struct daemon *daemon);

void
daemon_request_free (struct daemon_request *r);

struct daemon_request *
daemon_request_add (struct daemon_request *l, struct daemon_request *r);

struct daemon_request *
daemon_request_remove (struct daemon_request *l, pthread_t pt);

int
daemon_request_count (struct daemon_request *l);

#endif//DAEMON_REQUEST_H
