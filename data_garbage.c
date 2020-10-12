#include <time.h>
#include <stdio.h>
#include "data_garbage.h"
#include "data_structure.h"
#include "time.h"
#include "alloc.h"
#include "list.h"
#include "uri.h"
#include "interval.h"

struct garbageStats {
    unsigned int totalPeers;
    unsigned int totalTorrents;
    unsigned int totalTorrentsWithChangeLimit; // Торренты, для которых менялся level у структуры peerList

    unsigned int maxPeersInOneTorrent;
    unsigned int maxPeersInLeaf;
    unsigned int maxTorrentsInOneLeaf;

    unsigned int currentPeersInTorrent; // Кол-во пиров в торренте, которые остались после сборки мусора
    unsigned int currentPeersInLeaf; // Кол-во пиров в листе, которые были до сборки мусора
    unsigned int currentTorrentsInLeaf;

    unsigned int removedPeers;
    unsigned int removedTorrents;

    struct list *lastTorrentLeaf;
    struct list *lastPeerLeaf;

    long limitTime;

    struct item *currentTorrent;
};

/**
 * Обработка следующего пира
 * @param list
 * @param peer
 * @param args
 * @return
 */
unsigned char runGarbageCollectorCallbackCallback(struct list *list, struct item *peer, void *args) {
    struct garbageStats *garbageStats = args;
    struct peerDataL *peerDataL = peer->data;

    if (garbageStats->lastPeerLeaf != list) {
        garbageStats->currentPeersInLeaf = 0;
    }
    garbageStats->currentPeersInLeaf++;
    garbageStats->lastPeerLeaf = list;

    if (garbageStats->currentPeersInLeaf > garbageStats->maxPeersInLeaf) {
        garbageStats->maxPeersInLeaf = garbageStats->currentPeersInLeaf;
    }

    if (peerDataL->updateTime < garbageStats->limitTime) {
        // Удаляю пир

        // Правлю статистику
        struct torrentDataL *torrentDataL = garbageStats->currentTorrent->data;
        if (peerDataL->event == EVENT_ID_COMPLETED) {
            torrentDataL->complete--;
        } else {
            torrentDataL->incomplete--;
        }

        deleteItem(peer);
        garbageStats->removedPeers++;
    } else {
        garbageStats->totalPeers++;
        garbageStats->currentPeersInTorrent++;
    }

    return RETURN_CONTINUE;
}

/**
 * Обработка следующего торрента
 * @param list
 * @param torrent
 * @param args
 * @return
 */
unsigned char runGarbageCollectorCallback(struct list *list, struct item *torrent, void *args) {
    struct garbageStats *garbageStats = args;
    struct torrentDataL *torrentDataL = torrent->data;

    if (garbageStats->lastTorrentLeaf != list) {
        garbageStats->currentTorrentsInLeaf = 0;
    }
    garbageStats->currentTorrentsInLeaf++;
    garbageStats->lastTorrentLeaf = list;

    if (garbageStats->currentTorrentsInLeaf > garbageStats->maxTorrentsInOneLeaf) {
        garbageStats->maxTorrentsInOneLeaf = garbageStats->currentTorrentsInLeaf;
    }

    garbageStats->currentPeersInTorrent = 0;
    garbageStats->currentTorrent = torrent;

    mapList(torrentDataL->peerList, args, &runGarbageCollectorCallbackCallback);

    if (garbageStats->currentPeersInTorrent == 0) {
        freeList(torrentDataL->peerList, 1);

        deleteItem(torrent);

        garbageStats->removedTorrents++;
    } else {
        garbageStats->totalTorrents++;

        if (garbageStats->currentPeersInTorrent > garbageStats->maxPeersInOneTorrent) {
            garbageStats->maxPeersInOneTorrent = garbageStats->currentPeersInTorrent;
        }

        if (garbageStats->currentPeersInTorrent > LIMIT_PEERS_FOR_LEVEL_0) {
            garbageStats->totalTorrentsWithChangeLimit++;
        }
    }

    return RETURN_CONTINUE;
}

/**
 * Запуск сборщика мусора
 * @param block
 * @param torrentList
 */
void runGarbageCollectorL(struct block *block, struct list *torrentList) {
    struct garbageStats garbageStats = {};
    long now = time(NULL);
    garbageStats.limitTime = now - MAX_ALIVE_TIME - STEP_INTERVAL;

    unsigned long startTime = getStartTime();

    mapList(torrentList, &garbageStats, &runGarbageCollectorCallback);

    if (block == NULL)
        return;

    addFormatStringBlock(block, 4500,
                         "%.19s "
                         "%7d TP "
                         "%7d TT "
                         "%7d TL "

                         "%7d MPT "
                         "%7d MPL "
                         "%7d MTL "

                         "%7d RP "
                         "%7d RT "
                         "%7lu µs",
                         ctime((time_t *) &now),
                         garbageStats.totalPeers,
                         garbageStats.totalTorrents,
                         garbageStats.totalTorrentsWithChangeLimit,

                         garbageStats.maxPeersInOneTorrent,
                         garbageStats.maxPeersInLeaf,
                         garbageStats.maxTorrentsInOneLeaf,

                         garbageStats.removedPeers,
                         garbageStats.removedTorrents,
                         getDiffTime(startTime)
    );
}
