#ifndef SC6_SOCKET_UDP_H
#define SC6_SOCKET_UDP_H

#include "stats.h"
#include "data_change.h"
#include "queue.h"

struct serverUdpArgs {
    unsigned short port;
    struct stats *stats;
    struct firstByteData *firstByteData;
    unsigned int  *interval;
};

void *serverUdpHandler(void *args);

#endif //SC6_SOCKET_UDP_H
