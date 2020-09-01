#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <netinet/in.h>
#include "data.h"
#include "alloc.h"

int getPeerSize(struct peer *peer);

void int2ip(char *dest, unsigned int source);

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

    char middleBuffer[500] = {0};
    unsigned long sizeMiddleBuffer = 0;

    char ip[16];

    struct peer *currentPeer = peer;
    while (currentPeer != NULL) {

        if (query->compact) {
            // $peers .= pack('Nn', $array['ip'], $array['port']);
            memcpy(&peerString.data[peerString.size], &currentPeer->ip, 4);
            peerString.size += 4;
            memcpy(&peerString.data[peerString.size], &currentPeer->port, 2);
            peerString.size += 2;
        } else if (query->no_peer_id) {
            // "l"
            // "d"
            // "4:port" "i12345e"
            // "2:ip" "13:93.157.234.32"
            // "e"
            // "e"
            // $peers[] = array('ip' => long2ip($array['ip']), 'port' => intval($array['port']));
            int2ip(ip, currentPeer->ip);
            sprintf(middleBuffer,
                    "d"
                    "4:port" "i%de"
                    "2:ip" "%lu:%s"
                    "e",
                    htons(currentPeer->port),
                    strlen(ip),
                    ip
            );
            sizeMiddleBuffer = strlen(middleBuffer);
            memcpy(&peerString.data[peerString.size], middleBuffer, sizeMiddleBuffer);
            peerString.size += sizeMiddleBuffer;
        } else {
            // "l"
            // "d"
            // "4:port" "i12345e"
            // "7:peer id" "20:11111222223333344444"
            // "2:ip" "13:93.157.234.32"
            // "e"
            // "e"
            // $peers[] = array('ip' => long2ip($array['ip']), 'port' => intval($array['port']), 'peer id' => $array['peer_id']);

            sprintf(middleBuffer,
                    "d"
                    "4:port" "i%de"
                    "7:peer id" "20:",
                    htons(currentPeer->port)
            );
            sizeMiddleBuffer = strlen(middleBuffer);
            memcpy(&peerString.data[peerString.size], middleBuffer, sizeMiddleBuffer);
            peerString.size += sizeMiddleBuffer;

            memcpy(&peerString.data[peerString.size], &currentPeer->peer_id, 20);
            peerString.size += 20;

            int2ip(ip, currentPeer->ip);
            sprintf(middleBuffer,
                    "2:ip" "%lu:%s"
                    "e",
                    strlen(ip),
                    ip
            );
            sizeMiddleBuffer = strlen(middleBuffer);
            memcpy(&peerString.data[peerString.size], middleBuffer, sizeMiddleBuffer);
            peerString.size += sizeMiddleBuffer;
        }

        currentPeer = currentPeer->next;
    }

    if (query->compact) {
        sprintf(result->data,
                "d"
                "8:complete" "i0e"
                "10:downloaded" "i0e"
                "10:incomplete" "i1e"
                "8:interval" "i1800e"
                "12:min interval" "i1800e"
                "5:peers"
                "%lu:",
                peerString.size
        );
        result->size = strlen(result->data);
    } else {
        sprintf(result->data,
                "d"
                "8:complete" "i0e"
                "10:downloaded" "i0e"
                "10:incomplete" "i1e"
                "8:interval" "i1800e"
                "12:min interval" "i1800e"
                "5:peers"
                "l"
        );
        result->size = strlen(result->data);

    }

    memcpy(&result->data[result->size], peerString.data, peerString.size);
    result->size += peerString.size;

    if (!query->compact) {
        memcpy(&result->data[result->size], "e", 1);
        result->size += strlen("e");
    }

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

void int2ip(char *dest, unsigned int source) {
    char *src = (char *) &source;

    memset(dest, 0, 16);

    sprintf(dest, "%d.%d.%d.%d", src[0], src[1], src[2], src[3]);
}
