#ifndef SC6_SOCKET_TCP_H
#define SC6_SOCKET_TCP_H

#include "stats.h"
#include "sem.h"
#include "data_change.h"
#include "queue.h"

struct serverTcpArgs {
    char *port;
    struct stats *stats;
    struct rk_sema *sem;
    struct queue **first;
    struct firstByte *firstByte;
};

void *serverTcpHandler(void *args);

#endif //SC6_SOCKET_TCP_H
