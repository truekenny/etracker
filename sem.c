#include <stdint.h>
#include <stdio.h>
#include "sem.h"

#ifdef __APPLE__
#include <dispatch/dispatch.h>
#else
#include <semaphore.h>
#endif

/**
 * Структура семафора
 */
struct rk_sema {
#ifdef __APPLE__
    dispatch_semaphore_t    sem;
#else
    sem_t sem;
#endif
};

/**
 * Размер структуры семафора
 * @return
 */
int rk_sema_size() {
    return sizeof(struct rk_sema);
}

/**
 * Инициализация семафора
 * @param s
 * @param value
 */
void rk_sema_init(struct rk_sema *s, uint32_t value) {
#ifdef __APPLE__
    dispatch_semaphore_t *sem = &s->sem;

    *sem = dispatch_semaphore_create(value);
#else
    sem_init(&s->sem, 0, value);
#endif
}

/**
 * Блокирование семафора
 * @param s
 */
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

/**
 * Освобождение семафора
 * @param s
 */
void rk_sema_post(struct rk_sema *s) {
#ifdef __APPLE__
    dispatch_semaphore_signal(s->sem);
#else
    sem_post(&s->sem);
#endif
}
