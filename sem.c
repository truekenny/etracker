//
// Created by Kenny on 29.08.20.
//

#include <stdint.h>
#include <stdio.h>
#include "sem.h"

#ifdef __APPLE__
#include <dispatch/dispatch.h>
#else
#include <semaphore.h>
#endif

struct rk_sema {
#ifdef __APPLE__
    dispatch_semaphore_t    sem;
#else
    sem_t sem;
#endif
    int none;
    char str[200];
};


void rk_sema_init(struct rk_sema *s, uint32_t value) {
    printf("123\n");
    printf("sizeof struck = %lu\n", sizeof(struct rk_sema));
    printf("sizeof s = %lu\n", sizeof(s));
    printf("sizeof *s = %lu\n", sizeof(*s));
#ifdef __APPLE__
    dispatch_semaphore_t *sem = &s->sem;

    *sem = dispatch_semaphore_create(value);
#else
    sem_init(&s->sem, 0, value);
#endif
}

void rk_sema_wait(struct rk_sema *s) {
#ifdef __APPLE__
    dispatch_semaphore_wait(s->sem, DISPATCH_TIME_FOREVER);
#else
    int r;

    do {
        r = sem_wait(&s->sem);
    } while (r == -1 && errno == EINTR);
#endif
}

void rk_sema_post(struct rk_sema *s) {
#ifdef __APPLE__
    dispatch_semaphore_signal(s->sem);
#else
    sem_post(&s->sem);
#endif
}
