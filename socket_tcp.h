#ifndef SC6_SOCKET_TCP_H
#define SC6_SOCKET_TCP_H

#include "stats.h"
#include "sem.h"
#include "queue.h"
#include "socket_garbage.h"
#include "rps.h"
#include "list.h"

struct serverTcpArgs {
    unsigned short port;
    struct stats *stats;
    struct list *queueList;
    // struct firstByteData *firstByteData;
    struct list *torrentList;

    struct list *socketList;

    unsigned int *interval;
    struct rps *rps;
};

void *serverTcpHandler(void *args);

#endif //SC6_SOCKET_TCP_H
