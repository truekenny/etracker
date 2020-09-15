#ifndef SC6_SEM_H
#define SC6_SEM_H

#ifdef __APPLE__
#include <dispatch/dispatch.h>
#else
#include <semaphore.h>
#endif

#define CHECK_SEMAPHORE 0

/**
 * Структура семафора
 */
struct rk_sema {
#ifdef __APPLE__
    dispatch_semaphore_t    semaphoreQueue;
#else
    sem_t semaphoreQueue;
    #define uint32_t u_int32_t
#endif
};

/**
 * Инициализация семафора
 * @param s
 * @param value
 */
int rk_sema_init(struct rk_sema *s, uint32_t value);

/**
 * Блокирование семафора
 * @param s
 */
int rk_sema_wait(struct rk_sema *s);

/**
 * Освобождение семафора
 * @param s
 */
int rk_sema_post(struct rk_sema *s);

/**
 * Уничтожение семафора
 * @param s
 */
int rk_sema_destroy(struct rk_sema *s);

#endif //SC6_SEM_H
