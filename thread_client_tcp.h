#ifndef SC6_THREAD_CLIENT_TCP_H
#define SC6_THREAD_CLIENT_TCP_H

#include "stats.h"
#include "socket_garbage.h"
#include "rps.h"
#include "list.h"

/**
 * Аргументы переданные в поток
 */
struct clientTcpArgs {
    int threadNumber;
    struct list *queueList;
    // struct firstByteData *firstByteData;
    struct list *torrentList;
    struct stats *stats;

    int equeue;

    struct list *socketList;

    unsigned int *interval;
    struct rps *rps;
};

void *clientTcpHandler(void *);

#endif //SC6_THREAD_CLIENT_TCP_H
