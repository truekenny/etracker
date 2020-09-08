#include <memory.h>
#include <stddef.h>
#include <time.h>
#include <stdio.h>
#include "data_get.h"
#include "uri.h"
#include "alloc.h"

#define DEBUG 0

struct twoPointers *getTorrent(struct firstByteData *firstByte, unsigned char *hash) {
    struct twoPointers *twoPointers = c_calloc(1, sizeof(struct twoPointers));

    struct torrent *previous = {0};
    struct torrent *currentTorrent = firstByte->secondByteData[hash[0]].torrent[hash[1]];

    int compareValue;
    while (currentTorrent != NULL) {
        DEBUG &&
        printf("torrent %c%c%c\n", currentTorrent->info_hash[0], currentTorrent->info_hash[1], currentTorrent->info_hash[2]);

        compareValue = memcmp(currentTorrent->info_hash, hash, PARAM_VALUE_LENGTH);

        if (compareValue == 0) {
            DEBUG && printf("torrent ==\n");

            twoPointers->previous = previous;
            twoPointers->current = currentTorrent;

            return twoPointers;
        } else if (compareValue > 0) {
            DEBUG && printf("torrent >\n");

            break;
        }

        previous = currentTorrent;
        currentTorrent = currentTorrent->next;
    }

    twoPointers->previous = previous;

    return twoPointers;
}

struct twoPointers *getPeer(struct torrent *torrent, unsigned char *peer_id) {
    struct twoPointers *twoPointers = c_calloc(1, sizeof(struct twoPointers));

    if (torrent == NULL) {
        return twoPointers;
    }

    struct peer *previous = {0};
    struct peer *currentPeer = torrent->peer;

    int compareValue;
    while (currentPeer != NULL) {
        DEBUG && printf("peer %c%c%c\n", currentPeer->peer_id[0], currentPeer->peer_id[1], currentPeer->peer_id[2]);

        compareValue = memcmp(currentPeer->peer_id, peer_id, PARAM_VALUE_LENGTH);

        if (compareValue == 0) {
            DEBUG && printf("peer ==\n");

            twoPointers->previous = previous;
            twoPointers->current = currentPeer;

            return twoPointers;
        } else if (compareValue > 0) {
            DEBUG && printf("peer >\n");

            break;
        }

        previous = currentPeer;
        currentPeer = currentPeer->next;
    }

    twoPointers->previous = previous;

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
