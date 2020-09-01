#include <stddef.h>
#include <string.h>
#include <time.h>
#include "data.h"
#include "alloc.h"

void updatePeer(struct firstByte *firstByte, struct query *query) {
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

                break;
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
        }
    }
}
