#ifndef CLIENT_H
#define CLIENT_H

#include <semaphore.h>
struct client {
    //int                   id;
    int                   socket;
    /* We need to lock the socket to send atomic messages */
    sem_t                 socket_lock;
    char                  *addr; /* IPv4, IPv6, whatever... */
    pthread_t             thread_id;
    /* 
     * Many different requests will try and modify "requests", using request_add
     * () or request_remove (). That's why we need a semaphore.
     */
    struct request         *requests;
    sem_t                 req_lock;
    struct client         *next;
    struct client         *prev;
    /*
     * TODO :
     *
     * Add a semaphore in this struct (init. in client_new ()), to prevent
     * threads from modifying client->requests at the same time.
     *
     * The semaphore should be destroyed in client_free ().
     */
};

struct client *client_new     (int socket, char *addr);
void           client_free    (struct client *c);
struct client *client_add     (struct client *l, struct client *c);
int            client_numbers (struct client *l);
struct client *client_remove  (struct client *l, pthread_t pt);
int            client_send    (struct client *c, const char *msg);
#endif /* CLIENT_H */
