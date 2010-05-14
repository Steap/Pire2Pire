#ifndef THREAD_POOL_H
#define THREAD_POOL_H

/* NB : If you need a function to be called on job cancel, just add a cleanup
    function to the cleanup queue using pthread_cleanup_push () and
    pthread_cleanup_pop () in your job function

    Reminder :
        pthread_cleanup_pop (0)     <- Pops a function without executing it
        pthread_cleanup_pop (1)     <- Pops a function and executes it
*/

struct pool;

/* Create a pool of threads */
struct pool *
pool_create (unsigned int nb_threads);  /* number of threads */

/* Add a job to a pool queue */
int
pool_queue (struct pool *pool,          /* said pool */
            void * (*func) (void *),    /* job function */
            void *arg);                 /* argument passed to func */

/* Flush the pool queue from jobs whose arg is arg */
void
pool_flush_by_arg (struct pool *pool,   /* pool considered */
                    void *arg);         /* arg to seek */

/* Find a thread by its id in a pool and stop it */
void
pool_kill (struct pool *pool,   /* said pool */
            pthread_t tid);     /* thread to kill */

/* Destroy a pool, cancelling its threads and flushing remaining jobs */
void
pool_destroy (struct pool *pool);       /* pool to be destroyed */

#endif/*THREAD_POOL_H*/
