#include <memory.h>
#include <stddef.h>
#include <time.h>
#include "data_get.h"
#include "uri.h"
#include "alloc.h"

struct twoPointers *getTorrent(struct firstByte *firstByte, unsigned char *hash) {
    struct twoPointers *twoPointers = c_calloc(1, sizeof(struct twoPointers));

    struct torrent *previous = {0};
    struct torrent *currentTorrent = firstByte->secondByte[hash[0]].torrent[hash[1]];

    while (currentTorrent != NULL) {
        if (memcmp(currentTorrent->info_hash, hash, PARAM_VALUE_LENGTH) == 0) {
            twoPointers->previous = previous;
            twoPointers->current = currentTorrent;

            return twoPointers;
        }

        previous = currentTorrent;
        currentTorrent = currentTorrent->next;
    }

    return twoPointers;
}

struct twoPointers *getPeer(struct torrent *torrent, unsigned char *peer_id) {
    struct twoPointers *twoPointers = c_calloc(1, sizeof(struct twoPointers));

    if (torrent == NULL) {
        return twoPointers;
    }

    struct peer *previous = {0};
    struct peer *currentPeer = torrent->peer;

    while (currentPeer != NULL) {
        if (memcmp(currentPeer->peer_id, peer_id, PARAM_VALUE_LENGTH) == 0) {
            twoPointers->previous = previous;
            twoPointers->current = currentPeer;

            return twoPointers;
        }

        previous = currentPeer;
        currentPeer = currentPeer->next;
    }

    return twoPointers;
}

struct torrent *newTorrent(struct query *query) {
    struct torrent *torrent = c_calloc(1, sizeof(struct torrent));
    memcpy(torrent->info_hash, query->info_hash, PARAM_VALUE_LENGTH);

    return torrent;
}

struct peer *newPeer(struct query *query) {
    struct peer *peer = c_calloc(1, sizeof(struct peer));
    memcpy(peer->peer_id, query->peer_id, PARAM_VALUE_LENGTH);
    peer->ip = query->ip;
    peer->port = query->port;
    peer->updateTime = time(NULL);
    peer->event = query->event;

    return peer;
}
