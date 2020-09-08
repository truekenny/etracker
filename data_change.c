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
#include "data_get.h"

int getPeerSize(struct peer *peer);

struct torrent *deletePeer(struct firstByteData *firstByte, struct query *query) {
    struct twoPointers *twoTorrents = getTorrent(firstByte, query->info_hash);
    struct twoPointers *twoPeers = getPeer(twoTorrents->current, query->peer_id);

    // Есть такой торрент
    if (twoPeers->current) {
        struct peer *currentPeer = (struct peer *) twoPeers->current;
        struct torrent *currentTorrent = (struct torrent *) twoTorrents->current;

        torrentChangeStats(currentTorrent, currentPeer->event, query->event, -1);

        // Есть предыдущий
        if (twoPeers->previous) {
            struct peer *previousPeer = (struct peer *) twoPeers->previous;

            previousPeer->next = currentPeer->next;
        }
            // Найденный пир - первый в списке
        else {
            currentTorrent->peer = currentPeer->next;
        }

        c_free(currentPeer);
    }

    struct torrent *torrent = twoTorrents->current;

    c_free(twoPeers);
    c_free(twoTorrents);

    return torrent;
}

struct torrent *updatePeer(struct firstByteData *firstByte, struct query *query) {
    if (CHECK_SEMAPHORE) {
        printf("Check semaphore updatePeer(%d) begin = %d\n", 1, query->threadNumber);
        usleep(rand() % 5000 + 5000); // проверка работы семафора
        printf("Check semaphore updatePeer(%d) end = %d\n", 1, query->threadNumber);
    }

    struct twoPointers *twoTorrents = getTorrent(firstByte, query->info_hash);
    struct twoPointers *twoPeers = getPeer(twoTorrents->current, query->peer_id);

    // Нашелся пир
    if (twoPeers->current) {
        struct peer *currentPeer = (struct peer *) twoPeers->current;
        struct torrent *currentTorrent = (struct torrent *) twoTorrents->current;

        /*
         * Это должна быть самая первая строка, чтобы зафиксировать оба event;
         * Если event не меняется, то значения complete и incomplete не меняются
         */
        if (currentPeer->event != query->event)
            torrentChangeStats(currentTorrent, currentPeer->event, query->event, 1);

        currentPeer->ip = query->ip;
        currentPeer->port = query->port;
        currentPeer->updateTime = time(NULL);
        currentPeer->event = query->event;
    }
        // Нашелся только торрент
    else if (twoTorrents->current) {
        struct peer *peer = newPeer(query);
        struct torrent *currentTorrent = (struct torrent *) twoTorrents->current;
        // Подключаю пир
        if (twoPeers->previous == NULL) {
            // Добавляю пир в начало, так как предыдущего нет
            peer->next = currentTorrent->peer;
            currentTorrent->peer = peer;
        } else {
            // Добавляю пир после previous
            peer->next = ((struct peer *) twoPeers->previous)->next;
            ((struct peer *) twoPeers->previous)->next = peer;
        }

        torrentChangeStats(currentTorrent, peer->event, peer->event, 1);
    }
        // Нет даже торрента
    else {
        struct torrent *torrent = newTorrent(query);
        struct peer *peer = newPeer(query);
        // Подключаю пир
        torrent->peer = peer;
        // Подключаю торрент
        if (twoTorrents->previous == NULL) {
            // Добавляю торрент в начало, так как предыдущего нет
            torrent->next = firstByte->secondByteData[query->info_hash[0]].torrent[query->info_hash[1]];
            firstByte->secondByteData[query->info_hash[0]].torrent[query->info_hash[1]] = torrent;
        } else {
            // Добавляю торрент после previous
            torrent->next = ((struct torrent *) twoTorrents->previous)->next;
            ((struct torrent *) twoTorrents->previous)->next = torrent;
        }

        torrentChangeStats(torrent, peer->event, peer->event, 1);

        // result
        twoTorrents->current = torrent;
    }

    struct torrent *torrent = twoTorrents->current;

    c_free(twoPeers);
    c_free(twoTorrents);

    return torrent;
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
