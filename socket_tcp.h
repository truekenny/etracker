#ifndef SC6_SOCKET_TCP_H
#define SC6_SOCKET_TCP_H

#include "stats.h"
#include "sem.h"
#include "data.h"
#include "queue.h"

struct server_tcp {
    char *port;
    struct stats *stats;
    struct rk_sema *sem;
    struct queue **first;
    struct firstByte *firstByte;
};

void *serverTcp(void *args);

#endif //SC6_SOCKET_TCP_H
