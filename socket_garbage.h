#ifndef SC6_SOCKET_GARBAGE_H
#define SC6_SOCKET_GARBAGE_H

#include <time.h>
#include "stats.h"
#include "list.h"

struct socketData {
    int socket;
    int equeue;
    long time;
};

void updateSocketL(struct list *socketList,  int socket, int equeue, _Bool useSemaphore);

void deleteSocketItemL(struct item *item, struct stats *stats);

void deleteSocketL(struct list *socketList,  int socket, struct stats *stats);

#endif //SC6_SOCKET_GARBAGE_H
