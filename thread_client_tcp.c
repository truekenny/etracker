#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include <stdatomic.h>
#include "thread_client_tcp.h"
#include "sem.h"
#include "alloc.h"
#include "string.h"
#include "uri.h"
#include "data_garbage.h"
#include "socket.h"
#include "block.h"
#include "equeue.h"
#include "data.h"
#include "basic.h"
#include "thread.h"

/*
 * Если включить, это влияет очень сильно на CPU
 * Возможно из-за роста sockPool
 */
#define RECEIVED_MESSAGE_LENGTH 4000

struct deleteSocketListArgs {
    struct list *socketList;
    struct stats *stats;
};

unsigned char deleteSocketListCallback(struct list *list, struct item *item, void *args) {
    if (list == NULL) {
        // error: unused parameter 'list'
    }

    struct list *socketList = ((struct deleteSocketListArgs *) args)->socketList;
    struct stats *stats = ((struct deleteSocketListArgs *) args)->stats;
    int socket = *(int *) item->hash;

    deleteSocketL(socketList, socket, stats);
    deleteItem(item);

    return 0;
}

void processRead(struct clientTcpArgs *args, int currentSocket, struct list *deleteSocketList) {
    int threadNumber = args->threadNumber;
    struct list *torrentList = args->torrentList;
    struct stats *stats = args->stats;
    int equeue = args->equeue;
    struct list *socketList = args->socketList;
    _Atomic (unsigned int) *interval = args->interval;
    struct rps *rps = args->rps;
    struct block *authorizationHeader = args->authorizationHeader;
    unsigned int *maxPeersPerResponse = args->maxPeersPerResponse;
    unsigned short *socketTimeout = args->socketTimeout;
    unsigned char *keepAlive = args->keepAlive;
    char *charset = args->charset;

    unsigned char *pCurrentSocket = (unsigned char *) &currentSocket;

    // Read from socket.
    char readBuffer[RECEIVED_MESSAGE_LENGTH + 1];
    memset(readBuffer, 0, sizeof(readBuffer));
    ssize_t readSize = recv(currentSocket, readBuffer, RECEIVED_MESSAGE_LENGTH, MSG_NOSIGNAL | MSG_PEEK);

    // Запрос превышает лимит, прерываю такие сокеты
    if (readSize >= RECEIVED_MESSAGE_LENGTH) {
        struct block *block = initBlock();
        renderHttpMessage(block, 413, "Request Entity Too Large", 24, 0, *socketTimeout, stats, NULL, NULL);
        send_(currentSocket, block->data, block->size, stats);
        freeBlock(block);

        setHash(deleteSocketList, pCurrentSocket);

        stats->recv_failed++;

        return;
    }

    if (readSize == 0) {
        // Client close connection
        stats->recv_failed_read_0++;
        setHash(deleteSocketList, pCurrentSocket);

        return;
    }

    if (readSize < 0) {
        // Обычно Connection reset by peer, реже Bad File Descriptor, возможно еще что-нибудь
        stats->recv_failed++;
        stats->recv_failed_read_sub_0++;

        setHash(deleteSocketList, pCurrentSocket);

        return;
    }

    if (strstr(readBuffer, "\r\n\r\n") == NULL) {

        return;
    }

    updateRps(rps, RPS_TCP);

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

        setHash(deleteSocketList, pCurrentSocket);

        return;
    }

    _Bool isHttp = 0;
    int canKeepAlive = 0;
    struct block *writeBlock = initBlock();

    stats->recv_bytes += readSize;
    stats->recv_pass++;

    if (startsWith("GET ", readBuffer)) {
        isHttp = 1;
    }

    if (isHttp) {
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
                                  *socketTimeout, stats, NULL, NULL);
            } else if (!query.port) {
                renderHttpMessage(writeBlock, 400, "Field 'port' must be filled", 25, canKeepAlive,
                                  *socketTimeout, stats, NULL, NULL);
            } else {
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
                                  *socketTimeout, stats, NULL, NULL);
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
                                  canKeepAlive, *socketTimeout, stats, NULL, NULL);
                freeBlock(block);
            } else {
                renderHttpMessage(writeBlock, 401, "Authorization Failure", 21, canKeepAlive,
                                  *socketTimeout, stats, NULL, NULL);
            }
        } else if (startsWith("GET /stats", readBuffer)) {
            struct block *block = initBlock();

            formatStats(threadNumber, block, stats, *interval, rps);

            renderHttpMessage(writeBlock, 200, block->data, block->size, canKeepAlive,
                              *socketTimeout, stats, charset, "text/html");
            freeBlock(block);
        } else if (startsWith("GET / ", readBuffer)) {
            renderHttpMessage(writeBlock, 200,
                              "github.com/truekenny/etracker - open-source BitTorrent tracker\n", 63,
                              canKeepAlive, *socketTimeout, stats, NULL, NULL);
        } else if (startsWith("GET /scrape", readBuffer)) {
            stats->scrape++;

            struct query query = {};
            struct block *hashes = initBlock();
            struct block *block = initBlock();
            parseUri(&query, hashes, readBuffer);

            if (!hashes->size && !ENABLE_FULL_SCRAPE) {
                renderHttpMessage(writeBlock, 403, "Forbidden (Full Scrape Disabled)", 32, canKeepAlive,
                                  *socketTimeout, stats, NULL, NULL);
            } else {
                renderScrapeTorrentsPublic(block, torrentList, hashes, &query);
                renderHttpMessage(writeBlock, 200, block->data, block->size, canKeepAlive,
                                  *socketTimeout, stats, NULL, NULL);
            }

            freeBlock(hashes);
            freeBlock(block);
        } else if (startsWith("GET /favicon.", readBuffer)) {
            struct block *block = initBlock();
            addFileBlock(block, 2000, "web/favicon.ico");
            renderHttpMessage(writeBlock, 200, block->data, block->size, canKeepAlive,
                              *socketTimeout, stats, NULL, "image/x-icon");
            freeBlock(block);
        } else if (startsWith("GET /apple-touch-icon.", readBuffer)) {
            struct block *block = initBlock();
            addFileBlock(block, 2000, "web/apple-touch-icon.png");
            renderHttpMessage(writeBlock, 200, block->data, block->size, canKeepAlive,
                              *socketTimeout, stats, NULL, "image/png");
            freeBlock(block);
        } else if (startsWith("GET /robots.txt", readBuffer)) {
            struct block *block = initBlock();
            addFileBlock(block, 2000, "web/robots.txt");
            renderHttpMessage(writeBlock, 200, block->data, block->size, canKeepAlive,
                              *socketTimeout, stats, NULL, NULL);
            freeBlock(block);
        } else {
            // todo: Перенести чтение web файлов сюда
            renderHttpMessage(writeBlock, 404, "Page not found", 14, canKeepAlive,
                              *socketTimeout, stats, NULL, NULL);
        }

        if (canKeepAlive) {
            stats->keep_alive++;
        } else {
            stats->no_keep_alive++;
        }
    } // isHttp
    else {
        renderHttpMessage(writeBlock, 405, readBuffer, readSize, canKeepAlive,
                          *socketTimeout, stats, NULL, NULL);
    }

    send_(currentSocket, writeBlock->data, writeBlock->size, stats);
    // Ответ дан - удаляю
    freeBlock(writeBlock);

    if (!canKeepAlive) {
        setHash(deleteSocketList, pCurrentSocket);
    }
}

/**
 * This will handle connection for each client
 * @param args
 * @return
 */
void *clientTcpHandler(struct clientTcpArgs *args) {
    pthreadSetName(pthread_self(), "TCP worker");

    // int threadNumber = args->threadNumber;
    // struct list *torrentList = args->torrentList;
    struct stats *stats = args->stats;
    int equeue = args->equeue;
    struct list *socketList = args->socketList;
    // unsigned int *interval = args->interval;
    // struct rps *rps = args->rps;
    // struct block *authorizationHeader = args->authorizationHeader;
    // unsigned int *maxPeersPerResponse = args->maxPeersPerResponse;
    // unsigned short *socketTimeout = args->socketTimeout;
    // unsigned char *keepAlive = args->keepAlive;

    struct Eevent eevent;
    struct list *deleteSocketList = initList(NULL, 0, STARTING_NEST, sizeof(int), DISABLE_SEMAPHORE, LITTLE_ENDIAN);
    struct deleteSocketListArgs deleteSocketListArgs;
    deleteSocketListArgs.socketList = socketList;
    deleteSocketListArgs.stats = stats;

    while (1) {
        waitSemaphoreLeaf(socketList);

        int nev = checkEqueue(equeue, &eevent);

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
                setHash(deleteSocketList, pCurrentSocket);
            } else if (isRead(&eevent, index)) {
                processRead(args, currentSocket, deleteSocketList);
            }

            postSemaphoreLeaf(socketLeaf);
        } // for

        if (nev) {
            // Закрываю сокеты, которые требуют это
            mapList(deleteSocketList, &deleteSocketListArgs, &deleteSocketListCallback);
        }

        postSemaphoreLeaf(socketList);

        // Чтобы нормально работала подсветка кода в IDE
        if (rand() % 2 == 3) break;
    } // white 1

    freeList(deleteSocketList, 1);
    c_free(args);

    if (pthread_detach(pthread_self()) != 0) {
        perror("Could not detach thread");
    }

    return 0;
}
