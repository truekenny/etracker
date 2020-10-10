#ifndef SC6_SOCKET_UDP_H
#define SC6_SOCKET_UDP_H

#include <stdatomic.h>
#include "stats.h"
#include "rps.h"
#include "interval.h"
#include "geoip.h"

struct serverUdpArgs {
    unsigned short port;
    struct stats *stats;
    struct list *torrentList;
    struct interval *interval;

    struct rps *rps;
    long workers;
    unsigned int *maxPeersPerResponse;

    struct list *websockets;
    struct geoip *geoip;
};

void *serverUdpHandler(struct serverUdpArgs *args);

#endif //SC6_SOCKET_UDP_H
