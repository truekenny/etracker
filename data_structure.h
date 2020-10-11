#ifndef SC6_DATA_STRUCTURE_H
#define SC6_DATA_STRUCTURE_H

#include "sem.h"

// Максимальное кол-во пиров, на структуру peerList с level=0
#define LIMIT_PEERS_FOR_LEVEL_0 256

struct peerDataL {
    long updateTime;
    unsigned int ip;
    unsigned short port;
    unsigned char event;
};

struct torrentDataL {
    struct list *peerList;

    unsigned int complete;
    unsigned int incomplete;
    unsigned int downloaded;
};

#endif //SC6_DATA_STRUCTURE_H
