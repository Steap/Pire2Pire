#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include "client_handler.h"
#include "callback_argument.h"

#include "../util/logger.h"
#include "../util/string_util.h"
#include "client.h"
#include "request.h"
#include "requests.h"

#define BUFFSIZE                   128
#define DBG                          0

#define MAX_CLIENTS                  2
#define MAX_REQUESTS_PER_CLIENT      2

#define MAX_CLIENTS_REACHED         -1
#define MAX_REQUESTS_REACHED        -1

extern FILE *log_file;

/****** Some shit that might end up in util/ *********/
/* Given a socket, returns the message typed in */
static char*
get_request (int client_socket) {
    /* The message typed by the user might be longer than BUFFSIZE */
    char    *message;
    /* Number of chars written in message */ 
    int     ptr = 0;
    char    buffer[BUFFSIZE];
    int     n_received;

    /* BUFFSIZE + 1, so the terminating null byte ('\0') can be stored */
    if ((message = calloc (BUFFSIZE + 1, sizeof (char))) == NULL) {
        perror ("Allocating memory for message");
        return NULL;
    }

    while ((n_received = recv (client_socket, buffer, BUFFSIZE, 0)) != 0)
    {
        if (n_received < 0) {
            perror ("recv");
            // TODO: warn someone
            return NULL;
        }
        /*
         * Seems necessary, unless we can guarantee that we will only receive
         * well-formed strings... 
         */
        buffer[n_received] = '\0';

        strcpy (message + ptr, buffer);
        if((message = realloc (message, ptr + 2*BUFFSIZE + 1)) == NULL) {
            fprintf (stderr, "Error reallocating memory for message\n");
            // TODO: warn someone
            return NULL;
        }
        ptr += n_received;
        message[ptr] = '\0';

        if (strstr (buffer, "\n") != NULL) {
            #if 0
            if ((message = realloc (message, BUFFSIZE+1)) == NULL) {
                // TODO: warn someone
                return NULL;
            }
            #endif
            ptr = 0;
            string_remove_trailer (message);
            return message;
        }
    }
    return NULL;
}
/**** End of the shit that might end up in util/ *****/


struct client *clients = NULL;
/*
 * This semaphore is meant to prevent multiple clients from modifying "clients"
 * all at the same time.
 *
 * It should be initialized by the very first client, and sort of never be
 * destroyed.
 */
static sem_t          clients_lock;
#if 0
static void*
foo (void *a)
{
    struct request *r = (struct request *) a;
    struct request *tmp;
    char ans[100];

    sprintf (ans, 
             " < Number of requests : %d\n", 
             request_count (r->client->requests));
    send (r->client->socket, ans, strlen (ans), 0);
    for (tmp = r->client->requests; tmp; tmp = tmp->next) {
        sprintf (ans, "\t . %s\n", tmp->cmd);
        send (r->client->socket, ans, strlen (ans), 0);
    }
    sleep (1);
    
    sem_wait (&r->client->req_lock);
    r->client->requests = request_remove (r->client->requests, r->thread_id);
    sem_post (&r->client->req_lock);
    request_free (r);

    pthread_detach (pthread_self ());

    return NULL;
}
#endif
static void*
bar (void *a) { (void) a; sleep (100); return NULL;}
static void*
handle_requests (void *arg) {
    struct client                   *client;
    int                             r;
    /* The message typed by the user */
    char                            *message;
    void*                           (*callback) (void *);
    struct request                  *request;
    char                            answer[128];

    if (!(client = (struct client *) arg))
        goto out;
    
    log_success (log_file, "New client : %s", client->addr);

    for (;;)  {
        message  = NULL;
        callback = NULL;
        request  = NULL;

        message = get_request (client->socket);

        if (!message)
            goto out;

        /* Only request we're allowed to treat no matter how many requests are
         * currently being treated */
        if (strncmp (message, "quit", 4) == 0) 
            goto out;

        if (request_count (client->requests) == MAX_REQUESTS_PER_CLIENT) {
            sprintf (answer, " < Too many requests, mister, plz calm down\n");
            send (client->socket, answer, strlen (answer), 0);
            continue;
        }        

        /* Treating all the common requests */
        /* FIXME : use the IS_CMD macro */
        if (strncmp (message, "set", 3) == 0) 
            callback = &do_set;
        else if (strncmp (message, "info", 4) == 0)
            callback = &do_info;
        else if (strncmp (message, "bar", 3) == 0)
            callback = &bar;
        else 
            callback = &do_unknown_command;

        request = request_new (message, client);
        if (!request) {
            sprintf (answer, " < Failed to create a new request\n");
            send (client->socket, answer, strlen (answer), 0);
            continue;
        }
        
        client->requests = request_add (client->requests, request);
        r = pthread_create (&request->thread_id, NULL, callback, request);
        
        if (r < 0) {
            sprintf (answer, 
                    " < Could not treat your request for an unexpected \
                      reason.");
            log_failure (log_file, "Could not start new thread");
            continue;
        }
    }

out:
    log_success (log_file, "End of %s", client->addr);


    sem_wait (&clients_lock);
    clients = client_remove (clients, pthread_self ());
    sem_post (&clients_lock);

#if 1
    // Free : 1
    if (message) {
        free (message);
        message = NULL;
        log_failure (log_file, "hr : Freed message");
    }
#endif
    if (request) {
        log_failure (log_file, "hr : request_free ()");
        request_free (request);
        request = NULL;
    }
#if 1
    // Free : 2
    if (client) {
        log_failure (log_file, "hr : client_free ()");
        client_free (client);
        client = NULL;
        //log_success (log_file, "Freed client");
    }
#endif

    /* What if the same machine is connected from another client ? */
    pthread_detach (pthread_self ());
    
    return NULL;
}


void
handle_client (int client_socket, struct sockaddr_in *client_addr) {
    int            r;
    struct client *c;
    char          *addr;

    c    = NULL;
    addr = NULL;
    /*
     * Now, that is weird, but it seems even weirder to init that semaphore
     * outside the client_handler module (we could have done that in main.c).
     */
    static int xxx = 0;
    if (xxx == 0) {
        sem_init (&clients_lock, 0, 1); xxx = 1; }

    if (client_numbers (clients) == MAX_CLIENTS) {
        log_failure (log_file, "Too many clients");
        char answer[]= " < Too many clients right now\n";
        send (client_socket, answer, strlen (answer), 0);
        return;
    }

    addr = inet_ntoa (client_addr->sin_addr);
    if (addr) {
        c = client_new (client_socket, addr);
        free (addr);
    }

    if (!c)
        log_failure (log_file, "=/ :-( :'(");

    sem_wait (&clients_lock);
    clients = client_add (clients, c);
    sem_post (&clients_lock);

    if (!clients) {
        client_free (c);
        return;
    }

    log_failure (log_file, "NUMBER %d", client_numbers (clients));


    r = pthread_create (&c->thread_id, NULL, handle_requests, c);
    if (r < 0) {
        log_failure (log_file, "==> Could not create thread");
        client_remove (clients, c->thread_id);
        return;
    }
    
}

