#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include "sem.h"
#include "exit_code.h"

/**
 * Инициализация семафора
 * @param s
 * @param value
 */
int rk_sema_init(struct rk_sema *s, uint32_t value) {
#ifdef __APPLE__
    dispatch_semaphore_t *sem = &s->semaphoreQueue;

    *sem = dispatch_semaphore_create(value);
#else
    if (sem_init(&s->semaphoreQueue, 0, value)) {
        exitPrint(EXIT_CODE_SEMAPHORE_INIT, __FILE__, EXIT_CODE_PRINT_ERROR_YES);
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
    dispatch_semaphore_wait(s->semaphoreQueue, DISPATCH_TIME_FOREVER);
#else
    int r;

    do {
        r = sem_wait(&s->semaphoreQueue);
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
    dispatch_semaphore_signal(s->semaphoreQueue);
#else
    if (sem_post(&s->semaphoreQueue)) {
        exitPrint(EXIT_CODE_SEMAPHORE_POST, __FILE__, EXIT_CODE_PRINT_ERROR_YES);
    }
#endif
    return 0;
}

/**
 * Уничтожение семафора
 * @param s
 */
int rk_sema_destroy(struct rk_sema *s) {
#ifdef __APPLE__
    dispatch_release(s->semaphoreQueue);
#else
    if (sem_destroy(&s->semaphoreQueue)) {
        exitPrint(EXIT_CODE_SEMAPHORE_DESTROY, __FILE__, EXIT_CODE_PRINT_ERROR_YES);
    }
#endif
    return 0;
}
