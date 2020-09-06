#include <memory.h>
#include <stdio.h>
#include <arpa/inet.h>
#include "data_render.h"
#include "socket_udp_response_structure.h"

#define MAX_PEER_PER_RESULT 50

void int2ip(char *dest, unsigned int source);

void renderTorrent(struct block *block, struct torrent *torrent, unsigned char *hash, _Bool udp);

void renderTorrents(struct block *block, struct firstByte *firstByte, struct block *hashes, _Bool udp) {
    struct block *torrentBlock = initBlock();

    if (!udp) {
        addStringBlock(block, "d"
                              "5:files"
                              "d", 9);
    }

    if (hashes->size == 0 && ENABLE_FULL_SCRAPE) {
        int i, j;

        for (i = 0; i < 256; i++) {
            for (j = 0; j < 256; j++) {
                rk_sema_wait(&firstByte->secondByte[i].sem[j]);

                struct torrent *currentTorrent = firstByte->secondByte[i].torrent[j];

                while (currentTorrent != NULL) {
                    renderTorrent(torrentBlock, currentTorrent, NULL, udp);

                    currentTorrent = currentTorrent->next;
                }

                rk_sema_post(&firstByte->secondByte[i].sem[j]);
            }
        }
    } else {
        unsigned char hash[PARAM_VALUE_LENGTH];

        for (int index = 0; index < hashes->size / PARAM_VALUE_LENGTH; ++index) {
            memcpy(&hash, &hashes->data[index * PARAM_VALUE_LENGTH], PARAM_VALUE_LENGTH);

            rk_sema_wait(&firstByte->secondByte[hash[0]].sem[hash[1]]);

            struct torrent *currentTorrent = firstByte->secondByte[hash[0]].torrent[hash[1]];

            while (currentTorrent != NULL) {
                if (memcmp(hash, currentTorrent->info_hash, PARAM_VALUE_LENGTH) == 0) {
                    renderTorrent(torrentBlock, currentTorrent, NULL, udp);

                    break;
                }

                currentTorrent = currentTorrent->next;
            }

            // Торрент не нашелся - надо отбразить пустые данные
            if (currentTorrent == NULL) {
                renderTorrent(torrentBlock, NULL, hash, udp);
            }

            rk_sema_post(&firstByte->secondByte[hash[0]].sem[hash[1]]);
        }
    }

    addStringBlock(block, torrentBlock->data, torrentBlock->size);

    if (!udp) {
        addStringBlock(block, "e"
                              "e", 2);
    }

    freeBlock(torrentBlock);
}

void renderTorrent(struct block *block, struct torrent *torrent, unsigned char *hash, _Bool udp) {
    if (udp) {
        struct scrapeTorrentResponse scrapeTorrentResponse = {0};
        if (torrent) {
            scrapeTorrentResponse.seeders = htonl(torrent->complete);
            scrapeTorrentResponse.completed = htonl(torrent->downloaded);
            scrapeTorrentResponse.leechers = htonl(torrent->incomplete);
        }
        addStringBlock(block, &scrapeTorrentResponse, sizeof(struct scrapeTorrentResponse));
    } else {
        unsigned int complete = 0;
        unsigned int incomplete = 0;
        unsigned int downloaded = 0;

        addStringBlock(block, "20:", 3);

        if (torrent == NULL) {
            addStringBlock(block, hash, PARAM_VALUE_LENGTH);
        } else {
            addStringBlock(block, torrent->info_hash, PARAM_VALUE_LENGTH);
            complete = torrent->complete;
            incomplete = torrent->incomplete;
            downloaded = torrent->downloaded;
        }

        addFormatStringBlock(block, 1000, "d"
                                          "8:completei%de"
                                          "10:incompletei%de"
                                          "10:downloadedi%de"
                                          "e", complete, incomplete, downloaded);
    }
}

void renderPeers(struct block *block, struct torrent *torrent, struct query *query) {
    struct peer *peer = {0};
    unsigned int complete = 0;
    unsigned int incomplete = 0;
    unsigned int downloaded = 0;

    // После event=stopped торрента может и не быть
    if (torrent == NULL) {
        peer = NULL;
    } else {
        peer = torrent->peer;
        complete = torrent->complete;
        incomplete = torrent->incomplete;
        downloaded = torrent->downloaded;
    }

    int peerCounter = 0;

    struct block *peerBlock = initBlock();

    char ip[16];

    struct peer *currentPeer = peer;
    while (currentPeer != NULL) {
        peerCounter++;
        if (peerCounter > query->numwant || peerCounter > MAX_PEER_PER_RESULT)
            break;

        if (query->compact || query->udp) {
            addStringBlock(peerBlock, &currentPeer->ip, sizeof(int));
            addStringBlock(peerBlock, &currentPeer->port, sizeof(short));
        } else if (query->no_peer_id) {
            int2ip(ip, currentPeer->ip);
            addFormatStringBlock(peerBlock, 500,
                                 "d"
                                 "4:port" "i%de"
                                 "2:ip" "%lu:%s"
                                 "e",
                                 htons(currentPeer->port),
                                 strlen(ip),
                                 ip
            );
        } else {
            addFormatStringBlock(peerBlock, 500,
                                 "d"
                                 "4:port" "i%de"
                                 "7:peer id" "%d:",
                                 htons(currentPeer->port),
                                 PARAM_VALUE_LENGTH
            );

            addStringBlock(peerBlock, &currentPeer->peer_id, PARAM_VALUE_LENGTH);

            int2ip(ip, currentPeer->ip);
            addFormatStringBlock(peerBlock, 500,
                                 "2:ip" "%lu:%s"
                                 "e",
                                 strlen(ip),
                                 ip
            );
        }

        currentPeer = currentPeer->next;
    }

    if (query->udp) {}
    else if (query->compact) {
        addFormatStringBlock(block, 500,
                             "d"
                             "8:complete" "i%de"
                             "10:incomplete" "i%de"
                             "10:downloaded" "i%de"
                             "8:interval" "i%de"
                             "5:peers"
                             "%d:",
                             complete,
                             incomplete,
                             downloaded,
                             INTERVAL,
                             peerBlock->size
        );
    } else {
        addFormatStringBlock(block, 500,
                             "d"
                             "8:complete" "i%de"
                             "10:incomplete" "i%de"
                             "10:downloaded" "i%de"
                             "8:interval" "i%de"
                             "5:peers"
                             "l",
                             complete,
                             incomplete,
                             downloaded,
                             INTERVAL
        );
    }

    addStringBlock(block, peerBlock->data, peerBlock->size);

    if (!query->udp) {
        if (!query->compact) {
            addStringBlock(block, "e", sizeof(char));
        }

        addStringBlock(block, "e", sizeof(char));
    }

    freeBlock(peerBlock);
}

void int2ip(char *dest, unsigned int source) {
    char *src = (char *) &source;

    memset(dest, 0, 16);

    sprintf(dest, "%d.%d.%d.%d",
            (unsigned char) src[0],
            (unsigned char) src[1],
            (unsigned char) src[2],
            (unsigned char) src[3]);
}
