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

int getPeerSize(struct peer *peer);

void int2ip(char *dest, unsigned int source);

void torrentChangeStats(struct torrent *torrent, unsigned char oldEvent, unsigned char newEvent, char diff);

void runGarbageCollector(struct firstByte *firstByte) {
    int i, j;
    int removedPeers = 0, removedTorrents = 0;
    unsigned long now = time(NULL);
    unsigned long limitTime = now - INTERVAL * 2;

    unsigned long startTime = getStartTime();

    for (i = 0; i < 256; i++) {
        for (j = 0; j < 256; j++) {
            rk_sema_wait(&firstByte->secondByte[i].sem[j]);

            struct torrent *currentTorrent = firstByte->secondByte[i].torrent[j];
            struct torrent *previousTorrent = NULL;

            while (currentTorrent != NULL) {
                struct peer *currentPeer = currentTorrent->peer;
                struct peer *previousPeer = NULL;

                while (currentPeer != NULL) {
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

                        continue;
                    }

                    previousPeer = currentPeer;
                    currentPeer = currentPeer->next;
                }

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

                    continue;
                }

                previousTorrent = currentTorrent;
                currentTorrent = currentTorrent->next;
            }

            rk_sema_post(&firstByte->secondByte[i].sem[j]);
        }
    }

    printf("Garbage peers: %d, torrents: %d\n", removedPeers, removedTorrents);
    printf("Garbage time: %lu µs\n", getDiffTime(startTime));
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
        if (memcmp(currentTorrent->hash_info, query->info_hash, PARAM_VALUE_LENGTH) == 0) {
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
        memcpy(firstTorrent->hash_info, query->info_hash, PARAM_VALUE_LENGTH);

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
            if (memcmp(currentTorrent->hash_info, query->info_hash, PARAM_VALUE_LENGTH) == 0) {
                struct peer *currentPeer = currentTorrent->peer;
                while (currentPeer != NULL) {
                    // Пир нашелся
                    if (memcmp(currentPeer->peer_id, query->peer_id, PARAM_VALUE_LENGTH) == 0) {
                        // Самая первая строка, чтобы зафиксировать оба event
                        // Если event не меняется, то значения complete и incomplete не меняются
                        if (currentPeer->event != query->event)
                            torrentChangeStats(currentTorrent, currentPeer->event, query->event, 1);

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
            memcpy(torrent->hash_info, query->info_hash, PARAM_VALUE_LENGTH);

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

void renderPeers(struct result *result, struct torrent *torrent, struct query *query) {
    struct peer *peer = torrent->peer;

    int peerCounter = 0;

    result->data = c_calloc(1, 10000);

    struct result peerString = {0};
    peerString.data = c_calloc(1, 10000);

    char middleBuffer[500] = {0};
    unsigned long sizeMiddleBuffer;

    char ip[16];
    unsigned int complete = torrent->complete;
    unsigned int incomplete = torrent->incomplete;

    struct peer *currentPeer = peer;
    while (currentPeer != NULL) {
        peerCounter++;
        if (peerCounter > query->numwant || peerCounter > MAX_PEER_PER_RESULT)
            break;

        if (query->compact) {
            // $peers .= pack('Nn', $array['ip'], $array['port']);
            memcpy(&peerString.data[peerString.size], &currentPeer->ip, sizeof(int)); // 4
            peerString.size += sizeof(int);
            memcpy(&peerString.data[peerString.size], &currentPeer->port, sizeof(short)); // 2
            peerString.size += sizeof(short);
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
                    "7:peer id" "%d:",
                    htons(currentPeer->port),
                    PARAM_VALUE_LENGTH
            );
            sizeMiddleBuffer = strlen(middleBuffer);
            memcpy(&peerString.data[peerString.size], middleBuffer, sizeMiddleBuffer);
            peerString.size += sizeMiddleBuffer;

            memcpy(&peerString.data[peerString.size], &currentPeer->peer_id, PARAM_VALUE_LENGTH);
            peerString.size += PARAM_VALUE_LENGTH;

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
                "8:complete" "i%de"
                "10:incomplete" "i%de"
                "8:interval" "i%de"
                "5:peers"
                "%lu:",
                complete,
                incomplete,
                INTERVAL,
                peerString.size
        );
        result->size = strlen(result->data);
    } else {
        sprintf(result->data,
                "d"
                "8:complete" "i%de"
                "10:incomplete" "i%de"
                "8:interval" "i%de"
                "5:peers"
                "l",
                complete,
                incomplete,
                INTERVAL
        );
        result->size = strlen(result->data);

    }

    memcpy(&result->data[result->size], peerString.data, peerString.size);
    result->size += peerString.size;

    if (!query->compact) {
        memcpy(&result->data[result->size], "e", sizeof(char));
        result->size += sizeof(char);
    }

    memcpy(&result->data[result->size], "e", sizeof(char));
    result->size += sizeof(char);

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

    sprintf(dest, "%d.%d.%d.%d",
            (unsigned char)src[0],
            (unsigned char)src[1],
            (unsigned char)src[2],
            (unsigned char)src[3]);
}

/**
 * Меняет значение complete и incomplete торрента
 * @param torrent
 * @param oldEvent
 * @param newEvent
 * @param diff
 */
void torrentChangeStats(struct torrent *torrent, unsigned char oldEvent, unsigned char newEvent, char diff) {
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
