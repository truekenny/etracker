#ifndef SC6_THREAD_CLIENT_TCP_H
#define SC6_THREAD_CLIENT_TCP_H

#include <stdatomic.h>
#include "stats.h"
#include "socket_garbage.h"
#include "rps.h"
#include "list.h"
#include "interval.h"
#include "geoip.h"

/**
 * Аргументы переданные в поток
 */
struct clientTcpArgs {
    int threadNumber;
    struct list *torrentList;
    struct stats *stats;

    int equeue;

    struct list *socketList;

    struct interval *interval;
    struct rps *rps;
    struct block *authorizationHeader;

    unsigned int *maxPeersPerResponse;
    unsigned short *socketTimeout;
    unsigned char *keepAlive;
    char *charset;

    char *webRoot;

    struct list *websockets;
    struct geoip *geoip;
    char *xForwardedFor;
};

void *clientTcpHandler(struct clientTcpArgs *);

#endif //SC6_THREAD_CLIENT_TCP_H
