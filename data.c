#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "data.h"
#include "alloc.h"

int getPeerSize(struct peer *peer);

struct peer *updatePeer(struct firstByte *firstByte, struct query *query) {
    struct torrent *firstTorrent = firstByte->secondByte[query->info_hash[0]].torrent[query->info_hash[1]];

    // Торрентов нет - нужен новый торрент и пир
    if (firstTorrent == NULL) {
        firstTorrent = c_calloc(1, sizeof(struct torrent));
        memcpy(firstTorrent->hash_info, query->info_hash, 20);

        struct peer *peer = c_calloc(1, sizeof(struct peer));
        memcpy(peer->peer_id, query->peer_id, 20);
        peer->ip = query->ip;
        peer->port = query->port;
        peer->updateTime = time(NULL);
        firstTorrent->peer = peer;

        firstByte->secondByte[query->info_hash[0]].torrent[query->info_hash[1]] = firstTorrent;

        return peer;
    } else {
        struct torrent *currentTorrent = firstTorrent;

        while (currentTorrent != NULL) {
            // Торрент нашелся
            if (memcmp(currentTorrent->hash_info, query->info_hash, 20) == 0) {
                struct peer *currentPeer = currentTorrent->peer;
                while (currentPeer != NULL) {
                    // Пир нашелся
                    if (memcmp(currentPeer->peer_id, query->peer_id, 20) == 0) {
                        currentPeer->updateTime = time(NULL);

                        break;
                    }

                    currentPeer = currentPeer->next;
                }

                // Пир не найден - нужет новый пир
                if (currentPeer == NULL) {
                    struct peer *peer = c_calloc(1, sizeof(struct peer));
                    memcpy(peer->peer_id, query->peer_id, 20);
                    peer->ip = query->ip;
                    peer->port = query->port;
                    peer->updateTime = time(NULL);

                    peer->next = currentTorrent->peer;
                    currentTorrent->peer = peer;
                }

                return currentTorrent->peer;
                // break;
            }

            currentTorrent = currentTorrent->next;
        }

        // Торрент не найден - нужен новый и новый пир
        if (currentTorrent == NULL) {
            struct torrent *torrent = c_calloc(1, sizeof(struct torrent));
            memcpy(torrent->hash_info, query->info_hash, 20);

            struct peer *peer = c_calloc(1, sizeof(struct peer));
            memcpy(peer->peer_id, query->peer_id, 20);
            peer->ip = query->ip;
            peer->port = query->port;
            peer->updateTime = time(NULL);
            torrent->peer = peer;

            torrent->next = firstByte->secondByte[query->info_hash[0]].torrent[query->info_hash[1]];
            firstByte->secondByte[query->info_hash[0]].torrent[query->info_hash[1]] = torrent;

            return peer;
        }
    }

    // Эта строка недолжна быть достижима
    printf("This should not be seen: updatePeer\n");
    return NULL;
}

void getPeerString(struct result *result, struct peer *peer, struct query *query) {
    result->data = c_calloc(1, 10000);

    struct result peerString = {0};
    peerString.data = c_calloc(1, 10000);

    struct peer *currentPeer = peer;
    while (currentPeer != NULL) {

        if (query->compact) {
            // $peers .= pack('Nn', $array['ip'], $array['port']);
            memcpy(&peerString.data[peerString.size], &currentPeer->ip, 4);
            peerString.size += 4;
            memcpy(&peerString.data[peerString.size], &currentPeer->port, 2);
            peerString.size += 2;
        } else if (query->no_peer_id) {
            // $peers[] = array('ip' => long2ip($array['ip']), 'port' => intval($array['port']));
        } else {
            // $peers[] = array('ip' => long2ip($array['ip']), 'port' => intval($array['port']), 'peer id' => $array['peer_id']);
        }

        currentPeer = currentPeer->next;
    }

    sprintf(result->data,
            "d"
            "8:complete" "i0e"
            "10:downloaded" "i0e"
            "10:incomplete" "i1e"
            "8:interval" "i1800e"
            "12:min interval" "i1800e"
            "5:peers"
            // "6:Oov;09"
            "%lu:",
            // "e"
            peerString.size
    );
    result->size = strlen(result->data);

    memcpy(&result->data[result->size], peerString.data, peerString.size);
    result->size += peerString.size;

    memcpy(&result->data[result->size], "e", 1);
    result->size += strlen("e");

    c_free(peerString.data);
}

int getPeerSize(struct peer *peer) {
    struct peer *currentPeer;
    int size = 0;

    currentPeer = peer;

    while (currentPeer != NULL) {
        size++;

        currentPeer = currentPeer->next;
    }

    return size;
}

