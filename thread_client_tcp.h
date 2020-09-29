#ifndef SC6_THREAD_CLIENT_TCP_H
#define SC6_THREAD_CLIENT_TCP_H

#include <stdatomic.h>
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
    struct list *torrentList;
    struct stats *stats;

    int equeue;

    struct list *socketList;

    _Atomic(unsigned int) *interval;
    struct rps *rps;
    struct block *authorizationHeader;

    unsigned int *maxPeersPerResponse;
    unsigned short *socketTimeout;
    unsigned char *keepAlive;
};

void *clientTcpHandler(void *);

#endif //SC6_THREAD_CLIENT_TCP_H
