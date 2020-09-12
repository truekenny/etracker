#ifndef SC6_SOCKET_GARBAGE_H
#define SC6_SOCKET_GARBAGE_H

#include <time.h>
#include "stats.h"

struct socketPool {
    int socket;
    long time;

    struct socketPool *next;
};


void updateSocket(struct socketPool **socketPool, int socket);

void deleteSocket(struct socketPool **socketPool, int socket, struct stats *stats);

unsigned int runCollectSocket(struct socketPool **socketPool, struct stats *stats);

#endif //SC6_SOCKET_GARBAGE_H
