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

/*
    Create the pool, launch all threads
*/
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

/*
    Destroy the pool, cancelling all threads and flushing remaining jobs.
*/
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

sem_t           iter_lock;
unsigned int    iter;

/*
    Our job handler function, test purpose only
*/
static void * do_job (void *arg) {
    (void)arg;
    sem_wait (&iter_lock);
    printf (".");
    iter++;
    sem_post (&iter_lock);
    return NULL;
}

int main () {
    int i;
    struct pool *pool;

    sem_init (&iter_lock, 0, 1);
    iter = 0;

    pool = pool_create (NB_THREADS);
    if (!pool)
        exit (EXIT_FAILURE);
    for (i = 0; i < NB_JOBS; i++) {
        if (pool_queue (pool, do_job, NULL) < 0)
            break;
    }
    sleep (TIME_TO_LIVE);
    pool_destroy (pool);
    printf ("%d jobs achieved in less than %d seconds\n", iter, TIME_TO_LIVE);

    return EXIT_SUCCESS;
}

#if 0
/*
    Pops a job from the queue.
*/
struct job *job_pop () {
    struct job *popped;
    if (!jobq)
        return NULL;
    popped = jobq;
    jobq = jobq->next;
    return popped;
}

/*
    Pushes a job in the queue.
*/
void job_push (int job) {
    struct job *new_job = malloc (sizeof (struct job));
    if (!new_job) {
        perror ("malloc");
        exit (EXIT_FAILURE);
    }
    new_job->job = job;
    new_job->next = NULL;
    if (!jobq) {
        jobq = new_job;
    }
    else {
        struct job *tmp;
        for (tmp = jobq; tmp->next; tmp = tmp->next);
        tmp->next = new_job;
    }
}

/*
    Cancel routine, called when the threads are cancelled by the main one.
*/
void abort_thread (void *arg) {
    struct job  *job;
printf ("ABORTING\n");
    job = *((struct job **)arg);
    if (job) {
        free (job);
    }
printf ("ABORTED\n");
}

void my_pthread_mutex_unlock (void *mutex) {
printf ("UNLOCKING\n");
    PTHREAD_CHECK (pthread_mutex_unlock (mutex))
printf ("UNLOCKED\n");
}

/*
    Start routine for every thread.
*/
void *handle_jobs (void *arg) {
    struct job  *job = NULL;

    (void)arg;
    /* Defer cancellation (instead of being asynchronously cancelled) */
    PTHREAD_CHECK (pthread_setcanceltype (PTHREAD_CANCEL_DEFERRED, NULL))
    /* Enable cancellation (a thread can ask us to be cancelled) */
    PTHREAD_CHECK (pthread_setcancelstate (PTHREAD_CANCEL_ENABLE, NULL))
    /* On cancel, invoke abort_thread (&job) */
    pthread_cleanup_push (abort_thread, &job);

    for (;;) {
        /*
            Now this is tricky code. In order for us to unlock the mutex if we
            are cancelled while it is locked, we add an unlocking function to
            the cleanup stack.
            Remember never to PTHREAD_CHECK () neither pthread_cleanup_push ()
            nor pthread_cleanup_pop (), for those two are macros that dump
            accolades into your code, and thus shall not be encapsulated.
        */
        PTHREAD_CHECK (pthread_mutex_lock(jobq_mutex))
        pthread_cleanup_push (my_pthread_mutex_unlock, jobq_mutex);
        /*
            Since jobq_not_empty event is broadcasted to the whole pool, even
            if we wake up, jobq might still be empty. Hence the while ().
        */
        while (!jobq && !end) {
            pthread_cond_wait (jobq_not_empty, jobq_mutex);
        }
printf ("Is this the end?\n");
        if (end)
            break;
printf ("Nope\n");
        /* Pop a job from jobq */
        job = job_pop ();
        /* Unlock the mutex while popping cleanup stack */
        pthread_cleanup_pop (1);
        printf ("Doing job %d\n", job->job);
        free (job);
        /*
            Be sure to do this so that a cancellation knows whether the job has
            already been freed or not.
        */
        job = NULL;
        sleep (1); /* FIXME: For testing purposes */
    }
printf ("This is my end :(\n");
    /* Pop abort_thread from cleanup stack */
    pthread_cleanup_pop (1);
    return NULL;
}

/*
    Create the thread pool and the jobs, then return properly.
*/
int main () {
    int i;

    pthread_t thread_pool[NB_THREADS];

    /* Create the pool */
    for (i = 0; i < NB_THREADS; i++) {
        PTHREAD_CHECK (pthread_create (thread_pool + i, NULL, handle_jobs, NULL))
    }

    /* Add jobs and warn the pool each time */
    for (i = 0; i < NB_JOBS; i++) {
        job_push (i);
        PTHREAD_CHECK (pthread_cond_broadcast (jobq_not_empty))
    }

    /* Let's say the program stops after a few seconds */
    sleep (5);

printf ("1\n");
    end = 1;
    PTHREAD_CHECK (pthread_cond_broadcast (jobq_not_empty))
printf ("THIS IS THE END!\n");
    /* Cancel all threads */
    for (i = 0; i < NB_THREADS; i++) {
        PTHREAD_CHECK (pthread_cancel (thread_pool[i]))
    }
printf ("3\n");
    /* Wait for them to return after cancellation */
    for (i = 0; i < NB_THREADS; i++) {
        //PTHREAD_CHECK (pthread_join (thread_pool[i], NULL))
    }
printf ("4\n");
    /* Free all remaining jobs in the queue */
    struct job *tmp;
    while (jobq) {
        tmp = jobq->next;
        printf (" [-] FREE %d (flush)\n", jobq->job);
        free (jobq);
        jobq = tmp;
    }
printf ("5\n");
    return EXIT_SUCCESS;
}
#endif
