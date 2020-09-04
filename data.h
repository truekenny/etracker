#ifndef SC6_DATA_H
#define SC6_DATA_H

#include "uri.h"
#include "sem.h"
#include "block.h"

#define ENABLE_FULL_SCRAPE 0
#define INTERVAL 1800

struct result {
    char *data;
    unsigned long size;
};

struct peer {
    unsigned short port;
    unsigned int ip;
    char peer_id[20];
    long int updateTime;
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

void runGarbageCollector(struct firstByte *firstByte);

void initSem(struct firstByte *firstByte);
void waitSem(struct firstByte *firstByte, struct query *query);
void postSem(struct firstByte *firstByte, struct query *query);

struct torrent *deletePeer(struct firstByte *firstByte, struct query *query);
struct torrent *updatePeer(struct firstByte *firstByte, struct query *query);

void renderPeers(struct block *block, struct torrent *torrent, struct query *query);
void renderTorrents(struct block *block, struct firstByte *firstByte, struct block *hashes);

#endif //SC6_DATA_H
