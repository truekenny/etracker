#ifndef SC6_SOCKET_GARBAGE_H
#define SC6_SOCKET_GARBAGE_H

#include <time.h>

struct socketPool {
    int socket;
    long time;

    struct socketPool *next;
};


void updateSocket(struct socketPool **socketPool, int socket);

void deleteSocket(struct socketPool **socketPool, int socket);

void runCollectSocket(struct socketPool **socketPool);

#endif //SC6_SOCKET_GARBAGE_H
