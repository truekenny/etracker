#ifndef SC6_DATA_H
#define SC6_DATA_H

#include "uri.h"
#include "sem.h"

struct result {
    char *data;
    unsigned long size;
};

struct peer {
    unsigned short port;
    unsigned int ip;
    char peer_id[20];
    long int updateTime;

    struct peer *next;
};

struct torrent {
    char hash_info[20];
    struct peer *peer;

    struct torrent *next;
};

struct secondByte {
    struct torrent *torrent[256];
    struct rk_sema sem[256];
};

struct firstByte {
    struct secondByte secondByte[256];
};

void initSem(struct firstByte *firstByte);
void waitSem(struct firstByte *firstByte, struct query *query);
void postSem(struct firstByte *firstByte, struct query *query);

struct peer *deletePeer(struct firstByte *firstByte, struct query *query);
struct peer *updatePeer(struct firstByte *firstByte, struct query *query);

void getPeerString(struct result *result, struct peer *peer, struct query *query);

#endif //SC6_DATA_H
