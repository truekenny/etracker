#ifndef SC6_DATA_STRUCTURE_H
#define SC6_DATA_STRUCTURE_H

#include "sem.h"

struct peerDataL {
    unsigned short port;
    unsigned int ip;
    long updateTime;
    unsigned char event;
};

struct torrentDataL {
    struct list *peerList;

    unsigned int complete;
    unsigned int incomplete;
    unsigned int downloaded;
};

#endif //SC6_DATA_STRUCTURE_H
