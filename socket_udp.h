#ifndef SC6_SOCKET_UDP_H
#define SC6_SOCKET_UDP_H

#include "stats.h"
#include "queue.h"
#include "rps.h"

struct serverUdpArgs {
    unsigned short port;
    struct stats *stats;
    struct list *torrentList;
    unsigned int  *interval;

    struct rps *rps;
    long workers;
    unsigned int *maxPeersPerResponse;
};

void *serverUdpHandler(void *args);

#endif //SC6_SOCKET_UDP_H
