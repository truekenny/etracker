#ifndef SC6_SOCKET_UDP_H
#define SC6_SOCKET_UDP_H

#include <stdatomic.h>
#include "stats.h"
#include "rps.h"

struct serverUdpArgs {
    unsigned short port;
    struct stats *stats;
    struct list *torrentList;
    _Atomic (unsigned int) *interval;

    struct rps *rps;
    long workers;
    unsigned int *maxPeersPerResponse;
};

void *serverUdpHandler(struct serverUdpArgs *args);

#endif //SC6_SOCKET_UDP_H
