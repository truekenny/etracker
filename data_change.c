#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdlib.h>
#include "data_change.h"
#include "data_torrent_stat.h"
#include "alloc.h"
#include "uri.h"
#include "time.h"

int getPeerSize(struct peer *peer);

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
                    torrentChangeStats(currentTorrent, currentPeer->event, query->event, -1);
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
