#ifndef CLIENT_REQUEST_GET_H
#define CLIENT_REQUEST_GET_H


/* used by client_request_get.c to store 
 * seeder daemons when creating 1 job per daemon.
 */
struct daemon_list {
	struct daemon 		*daemon;
	int 				id;
	struct daemon_list	*next;
};


void *client_request_get (void *arg);
void *internal_request_get (void *arg);
#endif//CLIENT_REQUEST_GET_H
