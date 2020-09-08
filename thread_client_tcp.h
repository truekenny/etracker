#ifndef SC6_THREAD_CLIENT_TCP_H
#define SC6_THREAD_CLIENT_TCP_H

#include "data_change.h"
#include "stats.h"
#include "request.h"

/**
 * Аргументы переданные в поток
 */
struct clientTcpArgs {
    int threadNumber;
    struct rk_sema *semaphoreQueue;
    struct queue **queue;
    struct firstByteData *firstByteData;
    struct stats *stats;

    pthread_cond_t *signalRequest;
    pthread_mutex_t *mutexSignalRequest;
    struct request **firstRequest;
    struct request **lastRequest;
    struct rk_sema *semaphoreRequest;
};

void *clientTcpHandler(void *);

#endif //SC6_THREAD_CLIENT_TCP_H
