#ifndef SC6_SOCKET_TCP_H
#define SC6_SOCKET_TCP_H

#include <stdatomic.h>
#include "stats.h"
#include "sem.h"
#include "socket_garbage.h"
#include "rps.h"
#include "list.h"
#include "interval.h"
#include "geoip.h"

struct serverTcpArgs {
    unsigned short port;
    struct stats *stats;
    struct list *torrentList;

    struct list **socketLists;

    struct interval *interval;
    struct rps *rps;

    long workers;
    unsigned int *maxPeersPerResponse;
    unsigned short *socketTimeout;
    unsigned char *keepAlive;
    char *charset;

    struct list *websockets;
    struct geoip *geoip;
    char *xForwardedFor;
};

void *serverTcpHandler(struct serverTcpArgs *args);

#endif //SC6_SOCKET_TCP_H
