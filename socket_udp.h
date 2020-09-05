#ifndef SC6_SOCKET_UDP_H
#define SC6_SOCKET_UDP_H

#include "stats.h"
#include "data_change.h"
#include "queue.h"

struct serverUdpArgs {
    char *port;
    struct stats *stats;
    struct firstByte *firstByte;
};

void *serverUdpHandler(void *args);

#endif //SC6_SOCKET_UDP_H
