#include <pthread.h>
#include <stdio.h>
#include <memory.h>
#include <stdlib.h>
#include <stdatomic.h>
#include "thread_client_udp.h"
#include "alloc.h"
#include "uri.h"
#include "data_structure.h"
#include "socket_udp_structure.h"
#include "stats.h"
#include "string.h"
#include "data.h"
#include "thread.h"
#include "sem.h"

#define MSG_CONFIRM_ 0

void *clientUdpHandler(struct clientUdpArgs *args) {
    pthreadSetName(pthread_self(), "UDP worker");

    int sockAddrSize = sizeof(struct sockaddr_in);

    int serverSocket = args->serverSocket;
    struct list *torrentList = args->torrentList;
    struct stats *stats = args->stats;
    struct interval *interval = args->interval;

    pthread_cond_t *signalRequest = args->signalRequest;
    pthread_mutex_t *mutexSignalRequest = args->mutexSignalRequest;
    struct udpRequest **firstRequest = args->firstRequest;
    struct udpRequest **lastRequest = args->lastRequest;
    struct rk_sema *semaphoreRequest = args->semaphoreRequest;
    // unsigned int threadNumber = args->threadNumber;
    unsigned int *maxPeersPerResponse = args->maxPeersPerResponse;

    c_free(args);

    struct block *sendBlock = initBlock();
    struct block *scrapeBlock = initBlock();
    struct block *announceBlock = initBlock();
    struct block *hashesBlock = initBlock();

    while (1) {
        rk_sema_wait(semaphoreRequest);
        struct udpRequest *udpRequest = pullUdpRequest(firstRequest, lastRequest);
        rk_sema_post(semaphoreRequest);

        if (udpRequest == NULL) {
            pthread_mutex_lock(mutexSignalRequest);
            pthread_cond_wait(signalRequest, mutexSignalRequest);
            pthread_mutex_unlock(mutexSignalRequest);

            continue;
        }

        struct block *block = udpRequest->block;
        struct sockaddr_in clientAddr = udpRequest->clientAddr;
        unsigned int receiveCount = udpRequest->receiveCount;

        { // Проверка формата
            unsigned char connectRequestSize = sizeof(struct connectRequest);
            unsigned char connectResponseSize = sizeof(struct connectResponse);
            unsigned char announceRequestSize = sizeof(struct announceRequest);
            unsigned char scrapeRequestSize = sizeof(struct scrapeRequest);
            struct connectResponse connectResponse = {}; // ACTION_CONNECT = 0

            struct announceRequest *announceRequest = (struct announceRequest *) block->data;
            struct scrapeRequest *scrapeRequest = (struct scrapeRequest *) block->data;

            if (block->size == connectRequestSize) {
                stats->connect_udp++;

                struct connectRequest *connectRequest = (struct connectRequest *) block->data;
                if (connectRequest->protocol_id == PROTOCOL_ID && connectRequest->action == ACTION_CONNECT) {
                    connectResponse.transaction_id = connectRequest->transaction_id;
                    connectResponse.connection_id = receiveCount;

                    stats->sent_bytes_udp += connectResponseSize;

                    // printHex((char *) &connectResponse, connectResponseSize);

                    if (sendto(serverSocket, (const char *) &connectResponse, connectResponseSize,
                               MSG_CONFIRM_, (const struct sockaddr *) &clientAddr,
                               sockAddrSize) == -1) {
                        stats->send_failed_udp++;
                    } else {
                        stats->send_pass_udp++;
                    }
                }
            } else if (block->size >= announceRequestSize && htonl(announceRequest->action) == ACTION_ANNOUNCE) {
                stats->announce_udp++;

                // Аргументы потока
                struct query query = {};
                query.udp = 1;
                query.port = announceRequest->port;
                query.event = htonl(announceRequest->event);
                memcpy(query.info_hash, announceRequest->info_hash, PARAM_VALUE_LENGTH);
                memcpy(query.peer_id, announceRequest->peer_id, PARAM_VALUE_LENGTH);
                query.numwant = htonl(announceRequest->num_want);
                query.ip = clientAddr.sin_addr.s_addr;
                query.transaction_id = announceRequest->transaction_id;

                if (query.numwant > *maxPeersPerResponse)
                    query.numwant = *maxPeersPerResponse;

                { // аннонс
                    sendBlock = resetBlock(sendBlock);

                    struct list *leaf = getLeaf(torrentList, query.info_hash);
                    waitSemaphoreLeaf(leaf);

                    struct item *torrent;
                    if (query.event == EVENT_ID_STOPPED) {
                        torrent = deletePeerPublic(torrentList, &query);
                    } else {
                        torrent = setPeerPublic(torrentList, &query);
                    }
                    renderAnnouncePublic(sendBlock, announceBlock, torrent, &query, interval);

                    postSemaphoreLeaf(leaf);

                    stats->sent_bytes_udp += sendBlock->size;

                    // printHex(writeBlock->data, writeBlock->size);

                    if (sendto(serverSocket, sendBlock->data, sendBlock->size,
                               MSG_CONFIRM_, (const struct sockaddr *) &clientAddr,
                               sockAddrSize) == -1) {
                        perror("Sendto failed");
                        stats->send_failed_udp++;
                    } else {
                        stats->send_pass_udp++;
                    }
                } // аннонс

            } else if (block->size > scrapeRequestSize && htonl(scrapeRequest->action) == ACTION_SCRAPE) {
                stats->scrape_udp++;

                unsigned int hashCount = (block->size - sizeof(struct scrapeRequest)) / PARAM_VALUE_LENGTH;

                hashesBlock = resetBlock(hashesBlock);
                addStringBlock(hashesBlock,
                               &((char *) scrapeRequest)[sizeof(struct scrapeRequest)],
                               hashCount * PARAM_VALUE_LENGTH);

                {  // scrape
                    sendBlock = resetBlock(sendBlock);
                    struct query query = {};
                    query.udp = 1;
                    query.transaction_id = scrapeRequest->transaction_id;
                    renderScrapeTorrentsPublic(sendBlock, scrapeBlock, torrentList, hashesBlock, &query);

                    // printHex(writeBlock->data, writeBlock->size);
                    stats->sent_bytes_udp += sendBlock->size;

                    if (sendto(serverSocket, sendBlock->data, sendBlock->size,
                               MSG_CONFIRM_, (const struct sockaddr *) &clientAddr,
                               sockAddrSize) == -1) {
                        perror("Sendto failed");
                        stats->send_failed_udp++;
                    } else {
                        stats->send_pass_udp++;
                    }
                }  // scrape
            } // income packet scrape check
        } // Проверка формата

        freeUdpRequest(udpRequest);

        // Чтобы нормально работала подсветка кода в IDE
        if (rand() % 2 == 3) break;
    } // while 1

    freeBlock(sendBlock);
    freeBlock(scrapeBlock);
    freeBlock(announceBlock);
    freeBlock(hashesBlock);

    return 0;
}
