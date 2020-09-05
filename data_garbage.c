#include <time.h>
#include <printf.h>
#include "data_garbage.h"
#include "data_structure.h"
#include "data_torrent_stat.h"
#include "time.h"
#include "alloc.h"

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

    printf("%.19s GRBG: "
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
