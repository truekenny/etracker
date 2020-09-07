#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include "sem.h"

/**
 * Инициализация семафора
 * @param s
 * @param value
 */
int rk_sema_init(struct rk_sema *s, uint32_t value) {
#ifdef __APPLE__
    dispatch_semaphore_t *sem = &s->sem;

    *sem = dispatch_semaphore_create(value);
#else
    if (sem_init(&s->sem, 0, value)) {
        perror("Sem_init failed");
        exit(10);
    }
#endif
    return 0;
}

/**
 * Блокирование семафора
 * @param s
 */
int rk_sema_wait(struct rk_sema *s) {
#ifdef __APPLE__
    dispatch_semaphore_wait(s->sem, DISPATCH_TIME_FOREVER);
#else
    int r;

    do {
        r = sem_wait(&s->sem);
        if (r) {
            perror("Sem_wait failed");
        }
    } while (r == -1 && errno == EINTR);
#endif
    return 0;
}

/**
 * Освобождение семафора
 * @param s
 */
int rk_sema_post(struct rk_sema *s) {
#ifdef __APPLE__
    dispatch_semaphore_signal(s->sem);
#else
    if (sem_post(&s->sem)) {
        perror("Sem_post failed");
        exit(11);
    }
#endif
    return 0;

}
