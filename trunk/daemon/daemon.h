#ifndef DAEMON_H
#define DAEMON_H

#include <semaphore.h>      // sem_t

#include "daemon_request.h" // struct daemon_request

struct daemon {
    int                     socket;
    /* We need to lock the socket to send atomic messages */
    sem_t                   socket_lock;
    char                    *addr; /* IPv4, IPv6, whatever... */
    int                     port;   // Host short, use htons () if needed
    pthread_t               thread_id;
    /* 
     * Many different requests will try and modify "requests", using request_add
     * () or request_remove (). That's why we need a semaphore.
     */
    struct daemon_request   *requests;
    sem_t                   req_lock;
    struct daemon           *next;
    struct daemon           *prev;
};

struct daemon *daemon_new     (int socket, char *addr, int port);
void           daemon_free    (struct daemon *d);
struct daemon *daemon_add     (struct daemon *l, struct daemon *d);
int            daemon_numbers (struct daemon *l);
struct daemon *daemon_remove  (struct daemon *l, pthread_t pt);
int            daemon_send    (struct daemon *d, const char *msg);

#endif//DAEMON_H
