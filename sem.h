#ifndef SC6_SEM_H
#define SC6_SEM_H

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
 * Инициализация семафора
 * @param s
 * @param value
 */
void rk_sema_init(struct rk_sema *s, uint32_t value);

/**
 * Блокирование семафора
 * @param s
 */
void rk_sema_wait(struct rk_sema *s);

/**
 * Освобождение семафора
 * @param s
 */
void rk_sema_post(struct rk_sema *s);

#endif //SC6_SEM_H
