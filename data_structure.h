#ifndef SC6_DATA_STRUCTURE_H
#define SC6_DATA_STRUCTURE_H

#include <netinet/in.h>
#include "sem.h"

// Максимальное кол-во пиров, на структуру peerList с level=0
#define DATA_STRUCTURE_PEERS_FOR_LEVEL_0_LIMIT 256

#define DATA_STRUCTURE_PEER_PROTOCOL_TCP_BIT 0b1u
#define DATA_STRUCTURE_PEER_PROTOCOL_UDP_BIT 0b10u

struct peerDataL {
    long updateTime;
    struct in6_addr ip6;
    struct in6_addr ip4;
    unsigned short port;
    unsigned char event;
    unsigned char protocol;
    unsigned char ipVersion;
};

struct torrentDataL {
    struct list *peerList;

    unsigned int complete;
    unsigned int incomplete;
    unsigned int downloaded;
};

#endif //SC6_DATA_STRUCTURE_H
