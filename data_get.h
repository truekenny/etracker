#ifndef SC6_DATA_GET_H
#define SC6_DATA_GET_H

#include "data_structure.h"
#include "uri.h"

struct twoPointers {
    void *previous;
    void *current;
};

struct twoPointers *getTorrent(struct firstByte *firstByte, unsigned char *hash);

struct twoPointers *getPeer(struct torrent *torrent, unsigned char *peer_id);

struct torrent *newTorrent(struct query *query);

struct peer *newPeer(struct query *query);

#endif //SC6_DATA_GET_H
