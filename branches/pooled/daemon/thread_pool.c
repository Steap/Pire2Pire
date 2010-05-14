#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define NB_THREADS      42
#define NB_JOBS         160000
#define TIME_TO_LIVE    1

#define PTHREAD_CHECK(x) if (x) {                                           \
    fprintf (stderr, "PTHREAD_CHECK FAILED AT %s:%d\n", __FILE__, __LINE__);\
    exit (EXIT_FAILURE);                                                    \
}

struct job {
    struct job  *next;              /* Linked list */
    void *      (*func)(void *);    /* Function to call */
    void        *arg;               /* Its argument */
};

/*
    The job queue contains all available jobs to be done.
    Mutex lock jobq_mutex before any access to it.
*/
struct pool {
    pthread_mutex_t     mutex;
    pthread_cond_t      jobq_empty;         /* Is the jobq empty? */
    pthread_cond_t      no_more_threads;    /* Are there alive threads? */
    int                 nb_threads;
    int                 nb_alive_threads;
    pthread_t           *threads;           /* Tab of nb_threads elements */
    struct job          *jobq_head;         /* Linked list head */
    struct job          *jobq_tail;         /* And its tail */
    int                 destroy;            /* != 0 while destroying */
};

/*
    Called when a thread is cancelled. We're supposedly already locked.
*/
static void
worker_cleanup (void *arg) {
    struct pool *pool = (struct pool *)arg;

    --pool->nb_alive_threads;
    /*
        If this is the last cancelled thread, it must warn the waiting
        destroy thread that all threads were destroyed.
    */
    if (pool->nb_alive_threads == 0)
        PTHREAD_CHECK (pthread_cond_broadcast (&pool->no_more_threads))
    PTHREAD_CHECK (pthread_mutex_unlock (&pool->mutex))
}

/*
    If a thread is cancelled in the middle of a job, it will call this
    handler to get back in a locked state before full cancellation.
*/
static void
job_cleanup (void *arg) {
    struct pool *pool = (struct pool *)arg;

    PTHREAD_CHECK (pthread_mutex_lock (&pool->mutex))
}

static void *
worker_thread (void *arg) {
    struct pool *pool = (struct pool *)arg;
    struct job  *job;
    void *      (*func)(void *);

    PTHREAD_CHECK (pthread_mutex_lock (&pool->mutex))
    ++pool->nb_alive_threads;
    /* Call worker_cleanupi (pool) on cancellation */
    pthread_cleanup_push (worker_cleanup, pool);
    /* Defer cancellations (instead of cancelling asynchronously */
    PTHREAD_CHECK (pthread_setcanceltype (PTHREAD_CANCEL_DEFERRED, NULL))
    /* Enable cancellations (so that the main thread can kill us */
    PTHREAD_CHECK (pthread_setcancelstate (PTHREAD_CANCEL_ENABLE, NULL))
    for (;;) {
        /* Wait for a job (or destroy event) */
        while (!pool->jobq_head && !pool->destroy) {
            PTHREAD_CHECK (pthread_cond_wait (&pool->jobq_empty, &pool->mutex))
        }
        /* On destroy, leave */
        if (pool->destroy)
            break;
        /* Else, get the job */
        job = pool->jobq_head;
        pool->jobq_head = job->next;
        if (job == pool->jobq_tail)
            pool->jobq_tail = NULL;
        PTHREAD_CHECK (pthread_mutex_unlock (&pool->mutex))
        /* Retrieve job information */
        func = job->func;
        arg = job->arg;
        /* Call job_cleanup (pool) on cancellation to lock again */
        pthread_cleanup_push(job_cleanup, pool);
        free (job);
        /* Do the job */
        func (arg);
        pthread_cleanup_pop (1); /* pthread_mutex_lock (&pool->mutex) */
    }
    pthread_cleanup_pop (1); /* pthread_mutex_unlock (&pool->mutex) */

    return NULL;
}

/*
    Create all threads for given pool
*/
static int
create_threads (struct pool *pool) {
    int i;
    sigset_t    old_set;
    sigset_t    all_signals;

    /* Fill sigset with all signals */
    if (sigfillset(&all_signals) < 0)
        return -1;

    /* Create all threads with all signals masked */
    PTHREAD_CHECK (pthread_sigmask (SIG_SETMASK, &all_signals, &old_set))
    for (i = 0; i < pool->nb_threads; i++) {
        PTHREAD_CHECK (pthread_create (pool->threads + i,
                                        NULL, worker_thread, pool))
    }
    PTHREAD_CHECK (pthread_sigmask (SIG_SETMASK, &old_set, NULL))

    return 0;
}

/* API functions below, see thread_pool.h */

struct pool *
pool_create (unsigned int nb_threads) {
    struct pool *pool;

    pool = malloc (sizeof (struct pool));
    if (!pool)
        return NULL;

    PTHREAD_CHECK (pthread_mutex_init (&pool->mutex, NULL))
    PTHREAD_CHECK (pthread_cond_init (&pool->jobq_empty, NULL))
    PTHREAD_CHECK (pthread_cond_init (&pool->no_more_threads, NULL))
    pool->nb_threads = nb_threads;
    pool->nb_alive_threads = 0;
    pool->threads = malloc (nb_threads * sizeof (pthread_t));
    if (!pool->threads)
        return NULL;
    pool->jobq_head = NULL;
    pool->jobq_tail = NULL;
    pool->destroy = 0;

    if (create_threads (pool) < 0) {
        free (pool->threads);
        free (pool);
        return NULL;
    }

    return pool;
}

int
pool_queue (struct pool *pool, void * (*func) (void *), void *arg) {
    struct job  *job;

    job = malloc (sizeof (struct job));
    if (!job)
        return -1;
    job->next = NULL;
    job->func = func;
    job->arg = arg;

    pthread_mutex_lock (&pool->mutex);
    /* Add the job to the jobq */
    if (!pool->jobq_head)
        pool->jobq_head = job;
    else
        pool->jobq_tail->next = job;
    pool->jobq_tail = job;
    /* Warn the threads */
    pthread_cond_broadcast (&pool->jobq_empty);
    pthread_mutex_unlock (&pool->mutex);

    return 0;
}

void
pool_flush_by_arg (struct pool *pool, void *arg) {
    struct job  *removed;
    struct job  **pjob;

    pthread_mutex_lock (&pool->mutex);
    /* Use the pointer of pointer trick to avoid storing previous */
    pjob = &pool->jobq_head;
    /* Test pjob first in case it is NULL */
    while (pjob && *pjob) {
        /* Test *pjob against the matching condition */
        if ((*pjob)->arg == arg) {
            /* Store the reference to delete it */
            removed = *pjob;
            /* Change the pointer value to its next */
            *pjob = (*pjob)->next;
            /* Delete the previously stored reference */
            free (removed);
            /* Propagate the tail forward */
            if (*pjob)
                pool->jobq_tail = *pjob;
            /* No need to increment since we moved the next in place! */
        }
        else {
            /* Propagate the tail forward */
            if (*pjob)
                pool->jobq_tail = *pjob;
            /* Increment */
            pjob = &((*pjob)->next);
        }
    }
    pthread_mutex_unlock (&pool->mutex);
}

void
pool_kill (struct pool *pool, pthread_t tid) {
    int i;

    if (!pool)
        return;
    pthread_mutex_lock (&pool->mutex);
    /* Try to find it */
    for (i = 0; i < pool->nb_threads && pool->threads[i] != tid; i++);
    pthread_mutex_unlock (&pool->mutex);
    /* Didn't find it */
    if (i >= pool->nb_threads)
        return;
    /* Found */
    PTHREAD_CHECK (pthread_cancel (pool->threads[i]))
    PTHREAD_CHECK (pthread_join (pool->threads[i], NULL))
    pthread_mutex_lock (&pool->mutex);
    PTHREAD_CHECK (pthread_create (pool->threads + i,
                                    NULL, worker_thread, pool))
    pthread_mutex_unlock (&pool->mutex);
}

void
pool_destroy (struct pool *pool) {
    int i;
    struct job  *job;

    pthread_mutex_lock (&pool->mutex);
    pool->destroy = 1;
    /* Awakes everyone, having them checking destroy and exiting */
    pthread_cond_broadcast (&pool->jobq_empty);
    /* Now we cancel everyone */
    for (i = 0; i < pool->nb_threads; i++)
        pthread_cancel (pool->threads[i]);
    /*
        And wait for them to finish cancellation. The last one will set the
        condition variable no_more_threads and wake us.
    */
    while (pool->nb_alive_threads != 0)
        pthread_cond_wait (&pool->no_more_threads, &pool->mutex);
    pthread_mutex_unlock (&pool->mutex);

    /*
        In order to avoid a memory leak, we must do this...
    */
    for (i = 0; i < pool->nb_threads; i++)
        pthread_join (pool->threads[i], NULL);

    /*
        Let's free all remaining jobs
    */
    for (job = pool->jobq_head; job != NULL; job = pool->jobq_head) {
        pool->jobq_head = job->next;
        free (job);
    }

    free (pool->threads);
    free (pool);
}
