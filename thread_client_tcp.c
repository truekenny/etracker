#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include "thread_client_tcp.h"
#include "sem.h"
#include "alloc.h"
#include "queue.h"
#include "string.h"
#include "uri.h"
#include "data_garbage.h"
#include "socket.h"
#include "block.h"
#include "equeue.h"
#include "data.h"
#include "basic.h"

#define DEBUG 0
#define DEBUG_KQUEUE 0
#define QUEUE_ENABLE 0
/*
 * Если включить, это влияет очень сильно на CPU
 * Возможно из-за роста sockPool
 */
#define RECEIVED_MESSAGE_LENGTH 2000

void processRead(void *args, int currentSocket, struct item *socketItem) {
    int threadNumber = ((struct clientTcpArgs *) args)->threadNumber;
    struct list *queueList = ((struct clientTcpArgs *) args)->queueList;
    struct list *torrentList = ((struct clientTcpArgs *) args)->torrentList;
    struct stats *stats = ((struct clientTcpArgs *) args)->stats;
    int equeue = ((struct clientTcpArgs *) args)->equeue;
    struct list *socketList = ((struct clientTcpArgs *) args)->socketList;
    unsigned int *interval = ((struct clientTcpArgs *) args)->interval;
    struct rps *rps = ((struct clientTcpArgs *) args)->rps;
    struct block *authorizationHeader = ((struct clientTcpArgs *) args)->authorizationHeader;
    unsigned int *maxPeersPerResponse = ((struct clientTcpArgs *) args)->maxPeersPerResponse;
    unsigned short *socketTimeout = ((struct clientTcpArgs *) args)->socketTimeout;
    unsigned char *keepAlive = ((struct clientTcpArgs *) args)->keepAlive;

    DEBUG_KQUEUE && printf("thread_client_tcp.c: Read %d\n", currentSocket);
    // Read from socket.
    char readBuffer[RECEIVED_MESSAGE_LENGTH + 1];
    memset(readBuffer, 0, sizeof(readBuffer));
    ssize_t readSize = recv(currentSocket, readBuffer, RECEIVED_MESSAGE_LENGTH, MSG_NOSIGNAL | MSG_PEEK);
    DEBUG_KQUEUE && printf("thread_client_tcp.c: read %zu bytes fd=%d \n", readSize, currentSocket);

    // Запрос превышает лимит, прерываю такие сокеты
    if (readSize >= RECEIVED_MESSAGE_LENGTH) {
        // printf("recv has full buffer\n");

        struct block *block = initBlock();
        renderHttpMessage(block, 413, "Request Entity Too Large", 24, 0, *socketTimeout, stats);
        send_(currentSocket, block->data, block->size, stats);
        freeBlock(block);

        deleteSocketItemL(socketItem, stats);

        stats->recv_failed++;

        return;
    }

    if (readSize == 0) {
        // Client close connection

        stats->recv_failed_read_0++;

        deleteSocketItemL(socketItem, stats);

        return;
    }

    if (readSize < 0) {
        // Обычно Connection reset by peer, реже Bad File Descriptor, возможно еще что-нибудь
        stats->recv_failed++;
        stats->recv_failed_read_sub_0++;

        deleteSocketItemL(socketItem, stats);

        return;
    }

    if (strstr(readBuffer, "\r\n\r\n") == NULL) {

        return;
    }

    updateRps(rps);

    updateSocketL(socketList, currentSocket, equeue, 0);

    ssize_t beforeReadSize = readSize;

    // Сброс буфера, поскольку запрос полный, прочитать столько сколько было пикнуто
    readSize = recv(currentSocket, readBuffer, readSize, MSG_NOSIGNAL);

    if (beforeReadSize != readSize) {
        long now = time(NULL);

        stats->recv_failed++;
        stats->recv_failed_read_not_equal++;

        printf("%.19s beforeReadSize != readSize (%zd != %zd)\n",
               ctime((time_t *) &now), beforeReadSize, readSize);
        perror("Second recv failed");
        deleteSocketItemL(socketItem, stats);

        return;
    }

    DEBUG_KQUEUE && printf("thread_client_tcp.c: IN %d", threadNumber);

    if (QUEUE_ENABLE) {
        addQueueList(queueList, threadNumber);
    }

    DEBUG && printf("Handler: sock:%d number:%d\n", currentSocket, threadNumber);

    _Bool isHttp = 0;
    int canKeepAlive = 0;
    struct block *writeBlock = initBlock();

    DEBUG && printf("> %s", readBuffer);

    stats->recv_bytes += readSize;
    stats->recv_pass++;

    if (DEBUG && startsWith("stop", readBuffer)) {
        printf("STOP\n");
        exit(40);
    }

    if (startsWith("GET ", readBuffer)) {
        isHttp = 1;
        DEBUG && printf("isHttp = 1\n");
    }

    if (isHttp) {
        DEBUG && printf("Message complete\n");

        canKeepAlive = *keepAlive
                       && (
                               (
                                       (strstr(readBuffer, "HTTP/1.1") != NULL)
                                       && (strstr(readBuffer, "Connection: Close") == NULL)
                               )
                               || (strstr(readBuffer, "Connection: Keep-Alive") != NULL)
                       );

        if (startsWith("GET /announce", readBuffer)) {
            stats->announce++;

            struct query query = {};

            struct sockaddr_in peer = {};
            socklen_t socklen = sizeof(peer);
            getpeername(currentSocket, (struct sockaddr *) &peer, &socklen); // client
            query.ip = peer.sin_addr.s_addr;
            query.numwant = DEFAULT_NUM_WANT;
            query.event = EVENT_ID_STARTED;
            query.threadNumber = threadNumber;

            parseUri(&query, NULL, readBuffer);

            if (query.numwant > *maxPeersPerResponse)
                query.numwant = *maxPeersPerResponse;

            if (!query.has_info_hash) {
                renderHttpMessage(writeBlock, 400, "Field 'info_hash' must be filled", 25, canKeepAlive,
                                  *socketTimeout, stats);
            } else if (!query.port) {
                renderHttpMessage(writeBlock, 400, "Field 'port' must be filled", 25, canKeepAlive,
                                  *socketTimeout, stats);
            } else {
                // struct torrent *torrent;
                struct block *block = initBlock();

                struct list *leaf = getLeaf(torrentList, query.info_hash);
                waitSemaphoreLeaf(leaf);


                struct item *torrent;
                if (query.event == EVENT_ID_STOPPED) {
                    torrent = deletePeerPublic(torrentList, &query);
                } else {
                    torrent = setPeerPublic(torrentList, &query);
                }

                renderAnnouncePublic(block, torrent, &query, *interval);

                postSemaphoreLeaf(leaf);

                renderHttpMessage(writeBlock, 200, block->data, block->size, canKeepAlive,
                                  *socketTimeout, stats);
                freeBlock(block);
            }
        } else if (startsWith("GET /set", readBuffer)) {
            if (authorizationHeader->size == 0)
                getAuthorizationHeader(authorizationHeader);

            if (hasBasic(readBuffer, authorizationHeader->data)) {
                struct query query = {};
                parseUri(&query, NULL, readBuffer);

                struct block *block = initBlock();
                addFormatStringBlock(block, 500,
                                     "Before request:"
                                     "  keep_alive = %2u"
                                     "  interval = %4u"
                                     "  max_peers_response = %4u"
                                     "  socket_timeout = %2u\n",
                                     *keepAlive,
                                     *interval,
                                     *maxPeersPerResponse,
                                     *socketTimeout);

                if (query.interval)
                    *interval = query.interval;
                if (query.max_peers_per_response)
                    *maxPeersPerResponse = query.max_peers_per_response;
                if (query.socket_timeout)
                    *socketTimeout = query.socket_timeout;
                if (query.keep_alive)
                    *keepAlive = query.keep_alive == 1;

                addFormatStringBlock(block, 500,
                                     "After  request:"
                                     "  keep_alive = %2u"
                                     "  interval = %4u"
                                     "  max_peers_response = %4u"
                                     "  socket_timeout = %2u\n",
                                     *keepAlive,
                                     *interval,
                                     *maxPeersPerResponse,
                                     *socketTimeout);

                renderHttpMessage(writeBlock, 200,
                                  block->data, block->size,
                                  canKeepAlive, *socketTimeout, stats);
                freeBlock(block);
            } else {
                renderHttpMessage(writeBlock, 401, "Authorization Failure", 21, canKeepAlive,
                                  *socketTimeout, stats);
            }
        } else if (startsWith("GET /stats", readBuffer)) {
            struct block *block = initBlock();

            formatStats(threadNumber, block, stats, *interval, rps);

            if (QUEUE_ENABLE) {
                printQueueList(block, queueList);
            }

            renderHttpMessage(writeBlock, 200, block->data, block->size, canKeepAlive,
                              *socketTimeout, stats);
            freeBlock(block);
        } else if (DEBUG && startsWith("GET /garbage", readBuffer)) {
            struct block *block = initBlock();
            runGarbageCollectorL(block, torrentList);
            renderHttpMessage(writeBlock, 200, block->data, block->size, canKeepAlive,
                              *socketTimeout, stats);
            freeBlock(block);
        } else if (startsWith("GET / ", readBuffer)) {
            renderHttpMessage(writeBlock, 200,
                              "github.com/truekenny/etracker - open-source BitTorrent tracker\n", 63,
                              canKeepAlive, *socketTimeout, stats);
        } else if (startsWith("GET /scrape", readBuffer)) {
            stats->scrape++;

            struct query query = {};
            struct block *hashes = initBlock();
            struct block *block = initBlock();
            parseUri(&query, hashes, readBuffer);

            if (!hashes->size && !ENABLE_FULL_SCRAPE) {
                renderHttpMessage(writeBlock, 403, "Forbidden (Full Scrape Disabled)", 32, canKeepAlive,
                                  *socketTimeout, stats);
            } else {
                renderScrapeTorrentsPublic(block, torrentList, hashes, &query);
                renderHttpMessage(writeBlock, 200, block->data, block->size, canKeepAlive,
                                  *socketTimeout, stats);
            }

            freeBlock(hashes);
            freeBlock(block);
        } else {
            renderHttpMessage(writeBlock, 404, "Page not found", 14, canKeepAlive,
                              *socketTimeout, stats);
        }

        if (canKeepAlive) {
            stats->keep_alive++;
        } else {
            stats->no_keep_alive++;
        }
    } // isHttp
    else {
        renderHttpMessage(writeBlock, 405, readBuffer, readSize, canKeepAlive,
                          *socketTimeout, stats);
        DEBUG && printf("< %s", readBuffer);
    }

    if (QUEUE_ENABLE) {
        deleteQueueList(queueList, threadNumber);
    }

    DEBUG && printf("Recv bytes: %zu\n", readSize);
    DEBUG_KQUEUE && printf("thread_client_tcp.c: Write %d, keep=%d\n", threadNumber, canKeepAlive);

    send_(currentSocket, writeBlock->data, writeBlock->size, stats);
    // Ответ дан - удаляю
    freeBlock(writeBlock);

    if (!canKeepAlive) {
        deleteSocketItemL(socketItem, stats);
    }
}

/**
 * This will handle connection for each client
 * @param args
 * @return
 */
void *clientTcpHandler(void *args) {
    // int threadNumber = ((struct clientTcpArgs *) args)->threadNumber;
    // struct list *queueList = ((struct clientTcpArgs *) args)->queueList;
    // struct list *torrentList = ((struct clientTcpArgs *) args)->torrentList;
    struct stats *stats = ((struct clientTcpArgs *) args)->stats;
    int equeue = ((struct clientTcpArgs *) args)->equeue;
    struct list *socketList = ((struct clientTcpArgs *) args)->socketList;
    // unsigned int *interval = ((struct clientTcpArgs *) args)->interval;
    // struct rps *rps = ((struct clientTcpArgs *) args)->rps;
    // struct block *authorizationHeader = ((struct clientTcpArgs *) args)->authorizationHeader;
    // unsigned int *maxPeersPerResponse = ((struct clientTcpArgs *) args)->maxPeersPerResponse;
    // unsigned short *socketTimeout = ((struct clientTcpArgs *) args)->socketTimeout;
    // unsigned char *keepAlive = ((struct clientTcpArgs *) args)->keepAlive;

    struct Eevent eevent;

    while (1) {
        int nev = checkEqueue(equeue, &eevent);

        DEBUG_KQUEUE && printf("thread_client_tcp.c: go nev=%d\n", nev);

        for (int index = 0; index < nev; index++) {
            int currentSocket = getSocketEqueue(&eevent, index);

            unsigned char *pCurrentSocket = (unsigned char *) &currentSocket;
            struct list *socketLeaf = getLeaf(socketList, pCurrentSocket);
            waitSemaphoreLeaf(socketLeaf);
            struct item *socketItem = getHash(socketList, pCurrentSocket);

            if (socketItem == NULL) {
                long now = time(NULL);

                printf("%.19s thread_client_tpc.c: socketItem not found\n", ctime((time_t *) &now));
            } else if (isEof(&eevent, index)) {
                DEBUG_KQUEUE && printf("thread_client_tcp.c: Disconnect\n");

                deleteSocketItemL(socketItem, stats);
            } else if (isRead(&eevent, index)) {
                processRead(args, currentSocket, socketItem);
            }

            postSemaphoreLeaf(socketLeaf);
        } // for

        // Чтобы нормально работала подсветка кода в IDE
        if (rand() % 2 == 3) break;
    } // white 1

    if (pthread_detach(pthread_self()) != 0) {
        perror("Could not detach thread");
    }

    return 0;
}
