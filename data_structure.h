#ifndef SC6_DATA_STRUCTURE_H
#define SC6_DATA_STRUCTURE_H

#include "sem.h"

#define INTERVAL 1800

struct peer {
    unsigned short port;
    unsigned int ip;
    char peer_id[20];
    long updateTime;
    unsigned char event;

    struct peer *next;
};

struct torrent {
    char info_hash[20];
    struct peer *peer;
    unsigned int complete;
    unsigned int incomplete;
    unsigned int downloaded;

    struct torrent *next;
};

struct secondByte {
    struct torrent *torrent[256];
    struct rk_sema sem[256];
};

struct firstByte {
    struct secondByte secondByte[256];
};

#endif //SC6_DATA_STRUCTURE_H
