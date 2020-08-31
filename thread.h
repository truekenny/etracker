#ifndef SC6_THREAD_H
#define SC6_THREAD_H

/**
 * Аргументы переданные в поток
 */
struct args {
    int sock;
    int number;
    struct rk_sema *sem;
    struct queue **first;
};

void *connection_handler(void *);

#endif //SC6_THREAD_H
