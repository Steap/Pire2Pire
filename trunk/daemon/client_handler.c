#include <pthread.h>
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
#include "requests.h"

#define BUFFSIZE                   128
#define DBG                          0

#define MAX_CLIENTS                  1
#define MAX_REQUESTS_PER_CLIENT      2

#define MAX_CLIENTS_REACHED         -1
#define MAX_REQUESTS_REACHED        -1


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

            if ((message = realloc (message, BUFFSIZE+1)) == NULL) {
                // TODO: warn someone
                return NULL;
            }
            ptr = 0;
            string_remove_trailer (message);
            return message;
        }
    }
    return NULL;
}
/**** End of the shit that might end up in util/ *****/




/************** The client thingy *******************/
struct client {
    int                   id;
    int                   socket;
    char                  *addr; /* IPv4, IPv6, whatever... */
    pthread_t             requests[MAX_REQUESTS_PER_CLIENT];
    /* Maximum number of requests that have been running along at a given
     * time. */
    int                   n_init_requests;
};

static struct client *client_new (int id, int socket, char *addr) {
    struct client *c;
    c = malloc (sizeof (struct client));
    if (c == NULL)
        return NULL;
    c->id     = id;
    c->socket = socket;
    c->addr   = strdup (addr);
    /* It does not seem possible to properly init requests[] */
    c->n_init_requests = 0;
    return c;
}

static void client_free (struct client *c) {
    if (!c)
        return;
    if (c->addr) {
        free (c->addr);
        c->addr = NULL;
    }
    free (c);
    c = NULL;
}

/********** End of the client thingy ************/

/* Declared in main.c */
extern FILE      *log_file;

static int
is_active_thread (pthread_t *t)
{
    if (!t) {
        return 0;
    }
    return !pthread_kill (*t, 0);
}

static pthread_t client_threads[MAX_CLIENTS];
static int       n_init_clients = 0;

/*
 * find_next_available_client () and find_next_available_request ().
 *
 * We've got to keep track of the maximum number of threads running along at a
 * given time, so we never call is_active_thread () on a thread id that has
 * never been initialized. 
 *
 * One way not to do that brainfucked shit is to initialize
 * client_threads/client->requests with a particular value (an invalid one). But
 * it seems that the only way to initialize a pthread_t is to use the
 * pthread_create () function.
 */
static int
find_next_available_client () {
    int i;
    for (i = 0; i < n_init_clients; i++)
        if (!is_active_thread (client_threads+i))
            return i;

    if (i < MAX_CLIENTS) {
        n_init_clients += 1;
        return i;
    }
    
    return MAX_CLIENTS_REACHED;
}

static int
find_next_available_request (struct client *client) {
    int i;
    for (i = 0; i < client->n_init_requests; i++)
        if (!is_active_thread (client->requests + i))
            return i;

    if (i < MAX_REQUESTS_PER_CLIENT) {
        client->n_init_requests += 1;
        return i;
    }
    
    return MAX_REQUESTS_REACHED;
}

#if 0
/*
 * used for tests.
 */
void*
foo (void* a)
{
    (void) a;
    sleep (50);
    return NULL;
}
#endif

void*
handle_requests (void *arg) {
    struct client                   *client;
    int                             i, r;
    /* The message typed by the user */
    char                            *message;
    void*                           (*callback) (void *);
    struct callback_argument        *cba;

    message  = NULL;
    callback = NULL;
    cba      = NULL;

    if (!(client = (struct client *) arg))
        goto out;
    
    log_success (log_file, "New client : %s", client->addr);

    for (;;)  {
        cba = NULL;
        message = get_request (client->socket);
        if (!message)
            goto out;

        /* FIXME : use the IS_CMD macro */
        if (strncmp (message, "quit", 4) == 0) 
            goto out;
        else if (strncmp (message, "set", 3) == 0) 
            callback = &do_set;
        else 
            callback = &do_unknown_command;
//        callback=&foo;

        i = find_next_available_request (client); 
        if (i == MAX_REQUESTS_REACHED) {
            log_failure (log_file, 
                         " [Thread] Max reached reached for %s %s\n", 
                         client->addr, message);
            continue;
        }

        /*
         * That should never happen... But eh, you never know. Anyway, GCC could
         * remove that test... which might get us pwnd like the Linux folks ;-)
         */
        if (!callback)
            goto out;
        cba = cba_new (message, client->socket);
        if (!cba)
            goto out;
        /*
         * You should deifinitely NOT pthread_join here : if you do, the
         * execution of the calling thread will be suspended till the new thread
         * returns. And that might be *very* long.
         */
        r = pthread_create (client->requests+i, NULL, callback, cba);
        if (r < 0) {
            log_failure (log_file, "Could not start new thread");
            goto out;
        }
    }

out:
    /*
     * OK, this is major bullshit here. 
     *
     * We're freeing pointers, right ? So basically, memory that is in the
     * _heap_. Guess what ? It is shared between threads. So let's say we do
     * that :
     *
     * > set 
     * > quit
     *
     * really quickly. Or we simulate that by doing sleep (10) in do_set ().
     * We are going to free (cba) __before__ it is used in do_set(). Yep, that
     * is right : this is going to motherfucking fail. Two ways to solve that :
     *
     * 1) Pass a copy of cba (the actual __content__ of cba) to the callback
     * function when creating the thread. This is the dummy way, obviously. We
     * do not have 4GB machines so we can waste memory, but so we can do more
     * things. Like Java programming. Kr.
     *
     * 2) When the user types in "quit", and we /goto/ the "out" label, we
     * should pthread_join every request thread. We do not really care about
     * this particular thread being kept alive for a little bit more time. Plus,
     * we can get a return value from our request threads, which is cool, cuz, u
     * know, we can fill the logs with lots of "u failed" messages in case
     * something went wrong. That would require this thread knowing each and
     * every one of the threads it created, which means that every client should
     * have its own thread table.
     *
     * I'm on it.
     *
     * Cyril.
     *
     * On second thoughts :
     * Seems like, by doing cba = NULL, it could work. It *seems* to work, and
     * valgrind shuts the fuck up. But it's way too late for me to be sure about
     * anything.
     */
    log_success (log_file, "End of %s", client->addr);
    if (message) {
        free (message);
        message = NULL;
        //log_success (log_file, "Freed message");
    }

    if (client) {
        client_free (client);
        //log_success (log_file, "Freed client");
    }

    if (cba) {
        cba_free (cba);
        //log_success (log_file, "Freed cba");
    }
    /* What if the same machine is connected from another client ? */
//    pthread_detach (pthread_self ());
    return NULL;
}
void
handle_client (int client_socket, struct sockaddr_in *client_addr) {
    int             i, r;
    char            *addr;
    struct client   *client;
        
    addr = NULL;
    addr = inet_ntoa (client_addr->sin_addr);


    i = find_next_available_client ();
    if (i == MAX_CLIENTS_REACHED) {
        log_failure (log_file, 
                     "Could not handle client %s : already too many clients.",
                     addr);
        char answer[]= " < Too many clients right now\n";
        send (client_socket, answer, strlen (answer), 0);
        return;
    } 

    client = client_new (i, client_socket, addr);
    if (!client)
        return; 

    /* We may wanna free addr, but it might cause a crash (invalid free...). 
     * Investigation needed */
    r = pthread_create (client_threads+i, NULL, handle_requests, client);
    if (r < 0) {
        log_failure (log_file, "==> Could not create thread");
        client_free (client);
        return;
    }
}
