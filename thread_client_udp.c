#include <pthread.h>
#include <stdio.h>
#include <memory.h>
#include <stdlib.h>
#include "thread_client_udp.h"
#include "alloc.h"
#include "uri.h"
#include "data_sem.h"
#include "data_structure.h"
#include "data_change.h"
#include "data_render.h"
#include "socket_udp_structure.h"
#include "stats.h"
#include "string.h"

#define DEBUG 0
#define MSG_CONFIRM_ 0

void *clientUdpHandler(void *args) {
    int sockAddrSize = sizeof(struct sockaddr_in);
    struct clientUdpArgs *clientUdpArgs = (struct clientUdpArgs *) args;

    int serverSocket = clientUdpArgs->serverSocket;
    struct firstByteData *firstByteData = clientUdpArgs->firstByteData;
    struct stats *stats = clientUdpArgs->stats;
    unsigned int *interval = clientUdpArgs->interval;

    pthread_cond_t *signalRequest = clientUdpArgs->signalRequest;
    pthread_mutex_t *mutexSignalRequest = clientUdpArgs->mutexSignalRequest;
    struct udpRequest **firstRequest = clientUdpArgs->firstRequest;
    struct udpRequest **lastRequest = clientUdpArgs->lastRequest;
    struct rk_sema *semaphoreRequest = clientUdpArgs->semaphoreRequest;
    unsigned int threadNumber = clientUdpArgs->threadNumber;

    c_free(args);


    while (1) {
        rk_sema_wait(semaphoreRequest);
        struct udpRequest *udpRequest = pullUdpRequest(firstRequest, lastRequest);
        rk_sema_post(semaphoreRequest);

        if (udpRequest == NULL) {
            DEBUG && printf("thread_client_tcp.c: Before signal %d\n", threadNumber);
            pthread_mutex_lock(mutexSignalRequest);
            pthread_cond_wait(signalRequest, mutexSignalRequest);
            pthread_mutex_unlock(mutexSignalRequest);
            DEBUG && printf("thread_client_tcp.c: After signal %d\n", threadNumber);

            continue;
        } else {
            DEBUG && printf("thread_client_tcp.c: Ready request found %d, sock = %d\n", threadNumber,
                            udpRequest->clientAddr->sin_addr.s_addr);
        }

        struct block *block = udpRequest->block;
        struct sockaddr_in *clientAddr = udpRequest->clientAddr;
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
                DEBUG && printf("UDP Connect\n");
                stats->connect_udp++;

                struct connectRequest *connectRequest = (struct connectRequest *) block->data;
                if (connectRequest->protocol_id == PROTOCOL_ID && connectRequest->action == ACTION_CONNECT) {
                    connectResponse.transaction_id = connectRequest->transaction_id;
                    connectResponse.connection_id = receiveCount;

                    stats->sent_bytes_udp += connectResponseSize;
                    DEBUG && printHex((char *) &connectResponse, connectResponseSize);
                    if (sendto(serverSocket, (const char *) &connectResponse, connectResponseSize,
                               MSG_CONFIRM_, (const struct sockaddr *) clientAddr,
                               sockAddrSize) == -1) {
                        stats->send_failed_udp++;

                        if(DEBUG)
                            perror("sendto failed");
                    } else {
                        stats->send_pass_udp++;
                    }
                }
            } else if (block->size >= announceRequestSize && htonl(announceRequest->action) == ACTION_ANNOUNCE) {
                DEBUG && printf("UDP Announce\n");
                stats->announce_udp++;

                // Аргументы потока
                struct query *query = c_calloc(1, sizeof(struct query));
                query->udp = 1;
                query->port = announceRequest->port;
                query->event = htonl(announceRequest->event);;
                memcpy(query->info_hash, announceRequest->info_hash, PARAM_VALUE_LENGTH);
                memcpy(query->peer_id, announceRequest->peer_id, PARAM_VALUE_LENGTH);
                query->numwant = htonl(announceRequest->num_want);
                query->ip = clientAddr->sin_addr.s_addr;

                // todo: обработка announce
                { // аннонс
                    struct torrent *torrent = {0};
                    struct block *writeBlock = initBlock();

                    waitSem(firstByteData, query);
                    if (query->event == EVENT_ID_STOPPED) {
                        torrent = deletePeer(firstByteData, query);
                        DEBUG && printf("Delete UDP peer %s %s\n", query->info_hash, query->peer_id);
                    } else {
                        torrent = updatePeer(firstByteData, query);
                        DEBUG && printf("Update UDP peer %s %s\n", query->info_hash, query->peer_id);
                    }

                    struct announceHeadResponse announceHeadResponse = {};
                    announceHeadResponse.action = ntohl(ACTION_ANNOUNCE);
                    announceHeadResponse.transaction_id = announceRequest->transaction_id;
                    announceHeadResponse.interval = ntohl(*interval);
                    if (torrent != NULL) {
                        announceHeadResponse.leechers = ntohl(torrent->incomplete);
                        announceHeadResponse.seeders = ntohl(torrent->complete);
                    }
                    addStringBlock(writeBlock, &announceHeadResponse, sizeof(struct announceHeadResponse));

                    renderPeers(writeBlock, torrent, query, interval);
                    postSem(firstByteData, query);

                    stats->sent_bytes_udp += writeBlock->size;

                    DEBUG && printHex(writeBlock->data, writeBlock->size);
                    if (sendto(serverSocket, writeBlock->data, writeBlock->size,
                               MSG_CONFIRM_, (const struct sockaddr *) clientAddr,
                               sockAddrSize) == -1) {
                        perror("Sendto failed");
                        stats->send_failed_udp++;
                    } else {
                        stats->send_pass_udp++;
                    }

                    freeBlock(writeBlock);
                    c_free(query);

                } // аннонс

            } else if (block->size > scrapeRequestSize && htonl(scrapeRequest->action) == ACTION_SCRAPE) {
                DEBUG && printf("UDP Scrape\n");
                stats->scrape_udp++;

                unsigned int hashCount = (block->size - sizeof(struct scrapeRequest)) / PARAM_VALUE_LENGTH;
                DEBUG && printf("Hashes = %d\n", hashCount);

                struct block *hashes = initBlock();
                addStringBlock(hashes,
                               &((char *) scrapeRequest)[sizeof(struct scrapeRequest)],
                               hashCount * PARAM_VALUE_LENGTH);

                // todo: обработка scrape
                {  // scrape
                    struct block *writeBlock = initBlock();

                    struct scrapeHeadResponse scrapeHeadResponse = {};
                    scrapeHeadResponse.action = htonl(ACTION_SCRAPE);
                    scrapeHeadResponse.transaction_id = scrapeRequest->transaction_id;
                    addStringBlock(writeBlock, &scrapeHeadResponse, sizeof(struct scrapeHeadResponse));

                    renderTorrents(writeBlock, firstByteData, hashes, 1);


                    DEBUG && printHex(writeBlock->data, writeBlock->size);
                    if (sendto(serverSocket, writeBlock->data, writeBlock->size,
                               MSG_CONFIRM_, (const struct sockaddr *) clientAddr,
                               sockAddrSize) == -1) {
                        perror("Sendto failed");
                        stats->send_failed_udp++;
                    } else {
                        stats->send_pass_udp++;
                    }


                    freeBlock(hashes);
                    freeBlock(writeBlock);
                }  // scrape
            } // income packet scrape check

        } // Проверка формата


        freeUdpRequest(udpRequest);

        // Чтобы нормально работала подсветка кода в IDE
        if (rand() % 2 == 3) break;
    } // while 1

    return 0;
}
