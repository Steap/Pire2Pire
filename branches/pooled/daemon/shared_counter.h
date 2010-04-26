#ifndef SHARED_COUNTER_H
#define SHARED_COUNTER_H

struct shared_counter {
    int     count;
    sem_t   lock;
};

#endif/*SHARED_COUNTER_H*/
