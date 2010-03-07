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
#include "../util/logger.h"
#include "../util/string_util.h"

#define BUFFSIZE                   128
#define DBG                          0
#define MAX_REQUESTS_REACHED        -1

/* Numbers of threads that have been initialized */
static int       n_initialized_threads = 0;

/* Declared in main.c */
extern FILE      *log_file;

/* Might be a dummy implementation, we may want do something else one day */
static pthread_t threads[MAX_REQUESTS];

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
start (void *msg)
{
    log_success (log_file, 
                 " [Thread] Started new thread (%s) !", 
                (char *)msg);
    sleep (100);
    pthread_detach (pthread_self ());
    return NULL;
}

/*
 * We cannot create a thread with an id that is already used by another thread.
 * Given that we use a table to store threads, we need to find the index
 * of the first available id.
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

    if (i < MAX_REQUESTS) {
        n_initialized_threads += 1;
#if DBG
        fprintf (stdout, "i<MAX and n_init = %d\n", n_initialized_threads);
#endif
        return i;
    }
    
    return MAX_REQUESTS_REACHED;
}


#if 0
void
handle_request (int client_socket, struct sockaddr_in *client_sockaddr) {
    log_success (log_file, "OKOKOKOK");
//    log_file = fopen ("/tmp/a", "w");
//    log_success (log_file, "handling");
    int i, r;

    /* The message typed by the user might be longer than BUFFSIZE */
    char    *message;
    /* Number of chars written in message */ 
    int     ptr = 0;
    char    buffer[BUFFSIZE];
    int     n_received;
#if 1
    log_success (log_file, "Connection from %s at ", 
                           inet_ntoa (client_sockaddr->sin_addr));
#endif
    /* BUFFSIZE + 1, so the terminating null byte ('\0') can be stored */
    if ((message = calloc (BUFFSIZE + 1, sizeof (char))) == NULL) {
        perror ("Allocating memory for message");
        return;
    }
    
    while ((n_received = recv (client_socket, buffer, BUFFSIZE, 0)) != 0)
    {
        if (n_received < 0) {
            perror ("recv");
            // TODO: warn someone
            return;
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
            return;
        }
        ptr += n_received;
        message[ptr] = '\0';

        if (strstr (buffer, "\n") != NULL) {

            if ((message = realloc (message, BUFFSIZE+1)) == NULL) {
                fprintf (stderr, "Error reallocating memory for message");
                // TODO: warn someone
                return;
            }
            ptr = 0;
            break;
        }
    }


    i = find_next_available_thread (); 
    if (i == MAX_REQUESTS_REACHED) {
        log_failure (log_file, " [Thread] Max threads reached !\n");
        return;
    }

    r = pthread_create (threads+i, NULL, start, message);
    if (r < 0) {
        log_failure (log_file, "Could not start new thread");
        return;
    }

    free (message);
}
#endif
/*
 * Creates a new thread that handles the damn client.
 */
void*
foo (void *a) {
    (void) a;
    pthread_detach (pthread_self ());
    return NULL;
}
static void*
handle_request (void *arg) {
    int client_socket = *((int *) arg);
    int i, r;
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
   
    for (;;)  {
        while ((n_received = recv (client_socket, buffer, BUFFSIZE, 0)) != 0)
        {
            if (n_received < 0) {
                perror ("recv");
                // TODO: warn someone
                goto out;
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
                goto out;
            }
            ptr += n_received;
            message[ptr] = '\0';

            if (strstr (buffer, "\n") != NULL) {

                if ((message = realloc (message, BUFFSIZE+1)) == NULL) {
                    // TODO: warn someone
                    goto out;
                }
                ptr = 0;
                /**/
                string_remove_trailer (message);
                if (strcmp (message, "quit") == 0)
                    goto out;
                /**/
                break;
            }
        }


        i = find_next_available_thread (); 
        if (i == MAX_REQUESTS_REACHED) {
            log_failure (log_file, " [Thread] Max threads reached !\n");
            return NULL;
        }

        /*
         * You should deifinitely NOT pthread_join here : if you do, the
         * execution of the calling thread will be suspended till the new thread
         * returns. And that might be *very* long.
         */
        r = pthread_create (threads+i, NULL, start, message);
        if (r < 0) {
            log_failure (log_file, "Could not start new thread");
            return NULL;
        }
    }

out:
    log_success (log_file, "End of !");
    if (message)
        free (message);
    pthread_detach (pthread_self ());
    return NULL;
}

void
handle_client (int client_socket, struct sockaddr_in *client_addr) {
    int i, r;
    char *addr;
        
    addr = inet_ntoa (client_addr->sin_addr);

    log_success (log_file, "New client : %s", addr);

    i = find_next_available_thread (); 

    if (i == MAX_REQUESTS_REACHED) {
        log_failure (log_file, "==> COuld not handle another client");
        return;
    } 

    r = pthread_create (threads+i, NULL, handle_request, &client_socket);
    if (r < 0) {
        log_failure (log_file, "==> Could not create thread");
        return;
    }
}
