#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdlib.h>
#include "data.h"
#include "alloc.h"
#include "uri.h"
#include "time.h"

#define MAX_PEER_PER_RESULT 50

void renderTorrent(struct block *block, struct torrent* torrent);

int getPeerSize(struct peer *peer);

void int2ip(char *dest, unsigned int source);

void torrentChangeStats(struct torrent *torrent, unsigned char oldEvent, unsigned char newEvent, char diff);

void runGarbageCollector(struct firstByte *firstByte) {
    int i, j;
    unsigned int totalPeers = 0,
            totalTorrents = 0,
            maxPeersInOneTorrent = 0,
            maxTorrentsInOneHash = 0,
            currentPeersInOneTorrent = 0,
            currentTorrentsInOneHash = 0,
            removedPeers = 0,
            removedTorrents = 0;
    unsigned long now = time(NULL);
    unsigned long limitTime = now - INTERVAL * 2;

    unsigned long startTime = getStartTime();

    for (i = 0; i < 256; i++) {
        for (j = 0; j < 256; j++) {
            rk_sema_wait(&firstByte->secondByte[i].sem[j]);

            struct torrent *currentTorrent = firstByte->secondByte[i].torrent[j];
            struct torrent *previousTorrent = NULL;

            currentTorrentsInOneHash = 0;

            while (currentTorrent != NULL) {
                totalTorrents++;
                currentTorrentsInOneHash++;

                struct peer *currentPeer = currentTorrent->peer;
                struct peer *previousPeer = NULL;

                currentPeersInOneTorrent = 0;

                while (currentPeer != NULL) {
                    totalPeers++;
                    currentPeersInOneTorrent++;

                    if (currentPeer->updateTime < limitTime) {
                        // Пир просрочен
                        torrentChangeStats(currentTorrent, currentPeer->event, currentPeer->event, -1);

                        if (previousPeer == NULL) {
                            currentTorrent->peer = currentPeer->next;
                            c_free(currentPeer);
                            currentPeer = currentTorrent->peer;
                        } else {
                            previousPeer->next = currentPeer->next;
                            c_free(currentPeer);
                            currentPeer = previousPeer->next;
                        }
                        removedPeers++;
                        totalPeers--;

                        continue;
                    }

                    previousPeer = currentPeer;
                    currentPeer = currentPeer->next;
                }

                if (currentPeersInOneTorrent > maxPeersInOneTorrent)
                    maxPeersInOneTorrent = currentPeersInOneTorrent;

                if (currentTorrent->peer == NULL) {
                    // Надо удалить торрент, так как нет пиров
                    if (previousTorrent == NULL) {
                        firstByte->secondByte[i].torrent[j] = currentTorrent->next;
                        c_free(currentTorrent);
                        currentTorrent = firstByte->secondByte[i].torrent[j];
                    } else {
                        previousTorrent->next = currentTorrent->next;
                        c_free(currentTorrent);
                        currentTorrent = previousTorrent->next;
                    }
                    removedTorrents++;
                    totalTorrents--;

                    continue;
                }

                previousTorrent = currentTorrent;
                currentTorrent = currentTorrent->next;
            }

            if (currentTorrentsInOneHash > maxTorrentsInOneHash)
                maxTorrentsInOneHash = currentTorrentsInOneHash;

            rk_sema_post(&firstByte->secondByte[i].sem[j]);
        }
    }

    printf("%.24s Garbage: "
           "%7d TP %7d TT "
           "%7d MP %7d MT "
           "%7d RP %7d RT "
           "%7lu µs\n",
           ctime((time_t *) &now),
           totalPeers, totalTorrents,
           maxPeersInOneTorrent, maxTorrentsInOneHash,
           removedPeers, removedTorrents,
           getDiffTime(startTime));
}

void initSem(struct firstByte *firstByte) {
    int i, j;

    for (i = 0; i < 256; i++) {
        for (j = 0; j < 256; j++) {
            rk_sema_init(&firstByte->secondByte[i].sem[j], 1);
        }
    }
}

void waitSem(struct firstByte *firstByte, struct query *query) {
    struct rk_sema *sem = &firstByte->secondByte[query->info_hash[0]].sem[query->info_hash[1]];
    rk_sema_wait(sem);
}

void postSem(struct firstByte *firstByte, struct query *query) {
    struct rk_sema *sem = &firstByte->secondByte[query->info_hash[0]].sem[query->info_hash[1]];
    rk_sema_post(sem);
}

struct torrent *deletePeer(struct firstByte *firstByte, struct query *query) {
    struct torrent *currentTorrent = firstByte->secondByte[query->info_hash[0]].torrent[query->info_hash[1]];

    while (currentTorrent != NULL) {
        // Торрент нашелся
        if (memcmp(currentTorrent->info_hash, query->info_hash, PARAM_VALUE_LENGTH) == 0) {
            struct peer *currentPeer = currentTorrent->peer;
            struct peer *previous = {0};

            while (currentPeer != NULL) {
                // Пир нашелся
                if (memcmp(currentPeer->peer_id, query->peer_id, PARAM_VALUE_LENGTH) == 0) {
                    // Пир в списке первый
                    if (previous == NULL) {
                        currentTorrent->peer = currentPeer->next;
                    }
                        // Пир не первый
                    else {
                        previous->next = currentPeer->next;
                    }
                    torrentChangeStats(currentTorrent, currentPeer->event, currentPeer->event, -1);
                    c_free(currentPeer);

                    return currentTorrent;
                }

                previous = currentPeer;
                currentPeer = currentPeer->next;
            }

            // У торрента нет такого пира
            return currentTorrent;
        }

        currentTorrent = currentTorrent->next;
    }

    if (currentTorrent == NULL) {
        return NULL;
    }

    // Нет такого торрента
    return currentTorrent;
}

struct torrent *updatePeer(struct firstByte *firstByte, struct query *query) {
    struct torrent *firstTorrent = firstByte->secondByte[query->info_hash[0]].torrent[query->info_hash[1]];

    // Торрентов нет - нужен новый торрент и пир
    if (firstTorrent == NULL) {
        if (CHECK_SEMAPHORE) {
            printf("Check semaphore updatePeer(%d) begin = %d\n", 1, query->threadNumber);
            usleep(rand() % 5000 + 5000); // проверка работы семафора
            printf("Check semaphore updatePeer(%d) end = %d\n", 1, query->threadNumber);
        }
        firstTorrent = c_calloc(1, sizeof(struct torrent));
        memcpy(firstTorrent->info_hash, query->info_hash, PARAM_VALUE_LENGTH);

        struct peer *peer = c_calloc(1, sizeof(struct peer));
        memcpy(peer->peer_id, query->peer_id, PARAM_VALUE_LENGTH);
        peer->ip = query->ip;
        peer->port = query->port;
        peer->updateTime = time(NULL);
        peer->event = query->event;
        firstTorrent->peer = peer;

        firstByte->secondByte[query->info_hash[0]].torrent[query->info_hash[1]] = firstTorrent;

        torrentChangeStats(firstTorrent, peer->event, peer->event, 1);

        return firstTorrent;
    } else {
        if (CHECK_SEMAPHORE) {
            printf("Check semaphore updatePeer(%d) begin = %d\n", 2, query->threadNumber);
            usleep(rand() % 5000 + 5000); // проверка работы семафора
            printf("Check semaphore updatePeer(%d) end = %d\n", 2, query->threadNumber);
        }
        struct torrent *currentTorrent = firstTorrent;

        while (currentTorrent != NULL) {
            // Торрент нашелся
            if (memcmp(currentTorrent->info_hash, query->info_hash, PARAM_VALUE_LENGTH) == 0) {
                struct peer *currentPeer = currentTorrent->peer;
                while (currentPeer != NULL) {
                    // Пир нашелся
                    if (memcmp(currentPeer->peer_id, query->peer_id, PARAM_VALUE_LENGTH) == 0) {
                        // Самая первая строка, чтобы зафиксировать оба event
                        // Если event не меняется, то значения complete и incomplete не меняются
                        if (currentPeer->event != query->event)
                            torrentChangeStats(currentTorrent, currentPeer->event, query->event, 1);

                        currentPeer->ip = query->ip;
                        currentPeer->port = query->port;
                        currentPeer->updateTime = time(NULL);
                        currentPeer->event = query->event;

                        break;
                    }

                    currentPeer = currentPeer->next;
                }

                // Пир не найден - нужет новый пир
                if (currentPeer == NULL) {
                    struct peer *peer = c_calloc(1, sizeof(struct peer));
                    memcpy(peer->peer_id, query->peer_id, PARAM_VALUE_LENGTH);
                    peer->ip = query->ip;
                    peer->port = query->port;
                    peer->updateTime = time(NULL);
                    peer->event = query->event;

                    peer->next = currentTorrent->peer;
                    currentTorrent->peer = peer;

                    torrentChangeStats(currentTorrent, peer->event, peer->event, 1);
                }

                return currentTorrent;
                // break;
            }

            currentTorrent = currentTorrent->next;
        }

        // Торрент не найден - нужен новый и новый пир
        if (currentTorrent == NULL) {
            struct torrent *torrent = c_calloc(1, sizeof(struct torrent));
            memcpy(torrent->info_hash, query->info_hash, PARAM_VALUE_LENGTH);

            struct peer *peer = c_calloc(1, sizeof(struct peer));
            memcpy(peer->peer_id, query->peer_id, PARAM_VALUE_LENGTH);
            peer->ip = query->ip;
            peer->port = query->port;
            peer->updateTime = time(NULL);
            torrent->peer = peer;

            torrent->next = firstByte->secondByte[query->info_hash[0]].torrent[query->info_hash[1]];
            firstByte->secondByte[query->info_hash[0]].torrent[query->info_hash[1]] = torrent;

            torrentChangeStats(torrent, peer->event, peer->event, 1);

            return torrent;
        }
    }

    // Эта строка недолжна быть достижима
    printf("This should not be seen: updatePeer\n");
    return NULL;
}

void renderTorrents(struct block *block, struct firstByte *firstByte, struct block *hashes) {
    struct block *torrentBlock = initBlock();

    addStringBlock(block, "d"
                          "5:files"
                          "d", 9);

    if (hashes->size == 0) {
        int i, j;

        for (i = 0; i < 256; i++) {
            for (j = 0; j < 256; j++) {
                rk_sema_wait(&firstByte->secondByte[i].sem[j]);

                struct torrent *currentTorrent = firstByte->secondByte[i].torrent[j];

                while (currentTorrent != NULL) {
                    renderTorrent(torrentBlock, currentTorrent);

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
                if(memcmp(hash, currentTorrent->info_hash, PARAM_VALUE_LENGTH) == 0) {
                    renderTorrent(torrentBlock, currentTorrent);

                    break;
                }

                currentTorrent = currentTorrent->next;
            }

            rk_sema_post(&firstByte->secondByte[hash[0]].sem[hash[1]]);
        }
    }

    addStringBlock(block, torrentBlock->data, torrentBlock->size);

    addStringBlock(block, "e"
                          "e", 2);

    freeBlock(torrentBlock);
}



void renderTorrent(struct block *block, struct torrent* torrent) {
    addStringBlock(block, "20:", 3);
    addStringBlock(block, torrent->info_hash, PARAM_VALUE_LENGTH);
    addFormatStringBlock(block, 1000, "d"
                                      "8:completei%de"
                                      "10:incompletei%de"
                                      "10:downloadedi%de"
                                      "e", torrent->complete, torrent->incomplete, torrent->downloaded);
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

        if (query->compact) {
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

    if (query->compact) {
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

    if (!query->compact) {
        addStringBlock(block, "e", sizeof(char));
    }

    addStringBlock(block, "e", sizeof(char));

    freeBlock(peerBlock);
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

    sprintf(dest, "%d.%d.%d.%d",
            (unsigned char) src[0],
            (unsigned char) src[1],
            (unsigned char) src[2],
            (unsigned char) src[3]);
}

/**
 * Меняет значение complete и incomplete торрента
 * @param torrent
 * @param oldEvent
 * @param newEvent
 * @param diff
 */
void torrentChangeStats(struct torrent *torrent, unsigned char oldEvent, unsigned char newEvent, char diff) {
    if (newEvent == EVENT_ID_COMPLETED) {
        torrent->downloaded++;
    }

    if (oldEvent == EVENT_ID_COMPLETED && newEvent == EVENT_ID_COMPLETED) {
        torrent->complete += diff;
    } else if (oldEvent != EVENT_ID_COMPLETED && newEvent != EVENT_ID_COMPLETED) {
        torrent->incomplete += diff;
    } else if (oldEvent != EVENT_ID_COMPLETED && newEvent == EVENT_ID_COMPLETED) {
        torrent->incomplete -= diff;
        torrent->complete += diff;
    } else if (oldEvent == EVENT_ID_COMPLETED && newEvent != EVENT_ID_COMPLETED) {
        torrent->incomplete += diff;
        torrent->complete -= diff;
    }
}
