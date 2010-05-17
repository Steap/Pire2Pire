#ifndef SHARED_COUNTER_H
#define SHARED_COUNTER_H

struct shared_counter {
    int                 count;
    pthread_mutex_t     lock;
};

#endif/*SHARED_COUNTER_H*/
