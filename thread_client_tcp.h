#ifndef SC6_THREAD_CLIENT_TCP_H
#define SC6_THREAD_CLIENT_TCP_H

#include "data_change.h"
#include "stats.h"
#include "socket_garbage.h"

/**
 * Аргументы переданные в поток
 */
struct clientTcpArgs {
    int threadNumber;
    struct rk_sema *semaphoreQueue;
    struct queue **queue;
    struct firstByteData *firstByteData;
    struct stats *stats;

    int equeue;

    struct rk_sema *semaphoreSocketPool;
    struct socketPool **socketPool;

    unsigned int *interval;
};

void *clientTcpHandler(void *);

#endif //SC6_THREAD_CLIENT_TCP_H
