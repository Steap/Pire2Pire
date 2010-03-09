#ifndef REQUEST_H
#define REQUEST_H

struct client;

struct request {
    char *cmd;
    struct client *client;
    struct request *prev;
    struct request *next;
    pthread_t thread_id;
};

struct request  *request_new    (char *cmd, struct client *client);
void             request_free   (struct request *r);
struct request  *request_add    (struct request *l, struct request *r);
struct request  *request_remove (struct request *l, pthread_t pt);
int              request_count  (struct request *l); 
#endif /* REQUEST_H */
