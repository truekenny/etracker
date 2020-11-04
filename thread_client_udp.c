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
#include "websocket.h"
#include "socket.h"

#define THREAD_CLIENT_UDP__MSG_CONFIRM 0

void *clientUdpHandler(struct clientUdpArgs *args) {
    pthreadSetName(pthread_self(), "UDP worker");

    int sockAddrSize = sizeof(struct sockaddr_in6);

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
    struct list *websockets = args->websockets;
    struct geoip *geoip = args->geoip;

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
        struct sockaddr_in6 clientAddr = udpRequest->clientAddr;
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
                if (connectRequest->protocol_id == SOCKET_UDP_STRUCTURE_PROTOCOL_ID &&
                    connectRequest->action == SOCKET_UDP_STRUCTURE_ACTION_CONNECT) {
                    connectResponse.transaction_id = connectRequest->transaction_id;
                    connectResponse.connection_id = receiveCount;

                    stats->sent_bytes_udp += connectResponseSize;

                    // printHex((char *) &connectResponse, connectResponseSize);

                    if (sendto(serverSocket, (const char *) &connectResponse, connectResponseSize,
                               THREAD_CLIENT_UDP__MSG_CONFIRM, (const struct sockaddr *) &clientAddr,
                               sockAddrSize) == -1) {
                        incErrno(stats->send_errno_udp);
                        stats->send_failed_udp++;
                    } else {
                        stats->send_pass_udp++;
                    }
                }
            } else if (block->size >= announceRequestSize &&
                       htonl(announceRequest->action) == SOCKET_UDP_STRUCTURE_ACTION_ANNOUNCE) {
                stats->announce_udp++;

                // Аргументы потока
                struct query query = {};
                query.protocol = URI_QUERY_PROTOCOL_UDP;
                query.port = announceRequest->port;
                query.event = htonl(announceRequest->event);
                memcpy(query.info_hash, announceRequest->info_hash, URI_PARAM_VALUE_LENGTH);
                memcpy(query.peer_id, announceRequest->peer_id, URI_PARAM_VALUE_LENGTH);
                query.numwant = htonl(announceRequest->num_want);
                query.ip = clientAddr.sin6_addr;
                query.ipVersion = getIpVersion(&query.ip);
                query.transaction_id = announceRequest->transaction_id;

                if (query.numwant > *maxPeersPerResponse)
                    query.numwant = *maxPeersPerResponse;

                { // аннонс
                    sendBlock = resetBlock(sendBlock);

                    struct list *leaf = getLeaf(torrentList, query.info_hash);
                    waitSemaphoreLeaf(leaf);

                    struct item *torrent;
                    if (query.event == URI_EVENT_ID_STOPPED) {
                        torrent = deletePeerPublic(torrentList, &query);
                    } else {
                        torrent = setPeerPublic(torrentList, &query, DATA_STRUCTURE_PEER_PROTOCOL_UDP_BIT, stats);
                    }
                    renderAnnouncePublic(sendBlock, announceBlock, torrent, &query, interval);

                    postSemaphoreLeaf(leaf);

                    stats->sent_bytes_udp += sendBlock->size;

                    // printHex(writeBlock->data, writeBlock->size);

                    if (sendto(serverSocket, sendBlock->data, sendBlock->size,
                               THREAD_CLIENT_UDP__MSG_CONFIRM, (const struct sockaddr *) &clientAddr,
                               sockAddrSize) == -1) {
                        incErrno(stats->send_errno_udp);
                        stats->send_failed_udp++;
                    } else {
                        stats->send_pass_udp++;
                    }

                    if (query.ipVersion & SOCKET_VERSION_IPV4_BIT)
                        broadcast(websockets, geoip, clientAddr.sin6_addr, stats, WEBSOCKET_PROTOCOL_UDP);
                } // аннонс

            } else if (block->size > scrapeRequestSize &&
                       htonl(scrapeRequest->action) == SOCKET_UDP_STRUCTURE_ACTION_SCRAPE) {
                stats->scrape_udp++;

                unsigned int hashCount = (block->size - sizeof(struct scrapeRequest)) / URI_PARAM_VALUE_LENGTH;

                hashesBlock = resetBlock(hashesBlock);
                addStringBlock(hashesBlock,
                               &((char *) scrapeRequest)[sizeof(struct scrapeRequest)],
                               hashCount * URI_PARAM_VALUE_LENGTH);

                {  // scrape
                    sendBlock = resetBlock(sendBlock);
                    struct query query = {};
                    query.protocol = URI_QUERY_PROTOCOL_UDP;
                    query.transaction_id = scrapeRequest->transaction_id;
                    renderScrapeTorrentsPublic(sendBlock, scrapeBlock, torrentList, hashesBlock, &query);

                    // printHex(writeBlock->data, writeBlock->size);
                    stats->sent_bytes_udp += sendBlock->size;

                    if (sendto(serverSocket, sendBlock->data, sendBlock->size,
                               THREAD_CLIENT_UDP__MSG_CONFIRM, (const struct sockaddr *) &clientAddr,
                               sockAddrSize) == -1) {
                        incErrno(stats->send_errno_udp);
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
