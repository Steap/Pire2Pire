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
#define MAX_THREADS_REACHED        -1


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
    int                   socket;
    char                  *addr;
};

static struct client *client_new (int socket, char *addr) {
    struct client *c;
    c = malloc (sizeof (struct client));
    if (c == NULL)
        return NULL;
    c->socket = socket;
    c->addr   = strdup (addr);
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




/* Numbers of threads that have been initialized */
static int       n_initialized_threads = 0;

/* Declared in main.c */
extern FILE      *log_file;

/* Might be a dummy implementation, we may want do something else one day */
static pthread_t threads[MAX_THREADS];

static int
is_active_thread (pthread_t *t)
{
    if (!t) {
        return 0;
    }
    return !pthread_kill (*t, 0);
}

/*
 * Obviously, we won't use that function in real life.
 */
static void*
start (void *msg) {
    struct callback_argument *cba = (struct callback_argument *) msg;
    log_success (log_file, 
                 " [Thread] Started new thread (%s) !", 
                cba->cmd); 
    sleep (100);
    pthread_detach (pthread_self ());
    return NULL;
}

/*
 * We cannot create a thread with an id that is already used by another thread.
 * Given that we use a table to store threads, we need to find the index
 * of the first available id.
 *
 * Be careful, it does return an index an not an address. That might be worth
 * changing that one day, but eh, that ain't our biggest problem at the moment,
 * right ?
 */
static int
find_next_available_thread () {
    int i;
    /*
     * We've got to keep track of the maximum number of threads running along
     * at a given time, so we never call is_active_thread on a thread id that
     * has never been initialized.
     *
     * One way not to do that brainfucked shit is to initiliaze "threads" with a
     * particular value. But it seems that the only way to initialize a
     * pthread_t is to use the pthread_create () function.
     */
    for (i = 0; i < n_initialized_threads; i++)
        if (!is_active_thread (threads+i))
            return i;

    if (i < MAX_THREADS) {
        n_initialized_threads += 1;
#if DBG
        fprintf (stdout, "i<MAX and n_init = %d\n", n_initialized_threads);
#endif
        return i;
    }
    
    return MAX_THREADS_REACHED;
}

/*
 * Handle requests by creating a new thread for each and every one of them.
 */
static void*
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
            callback = &start;

        i = find_next_available_thread (); 
        if (i == MAX_THREADS_REACHED) {
            log_failure (log_file, " [Thread] Max threads reached !\n");
            goto out;
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
        r = pthread_create (threads+i, NULL, callback, cba);
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
        log_success (log_file, "Freed message");
    }

    if (client) {
        client_free (client);
        log_success (log_file, "Freed client");
    }

    if (cba) {
        cba_free (cba);
        log_success (log_file, "Freed cba");
    }
    return NULL;
    /* What if the same machine is connected from another client ? */
//    pthread_detach (pthread_self ());
}

void
handle_client (int client_socket, struct sockaddr_in *client_addr) {
    int             i, r;
    char            *addr;
    struct client   *client;
        
    addr = inet_ntoa (client_addr->sin_addr);

    log_success (log_file, "New client : %s", addr);

    i = find_next_available_thread (); 
    if (i == MAX_THREADS_REACHED) {
        log_failure (log_file, "==> COuld not handle another client");
        return;
    } 

    client = client_new (client_socket, addr);
    if (!client)
        return; 

    r = pthread_create (threads+i, NULL, handle_requests, client);
    if (r < 0) {
        log_failure (log_file, "==> Could not create thread");
        client_free (client);
        return;
    }
}
