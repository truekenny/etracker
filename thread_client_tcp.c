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
#include "data_change.h"
#include "data_sem.h"
#include "data_garbage.h"
#include "data_render.h"
#include "socket.h"
#include "block.h"
#include "equeue.h"

#define DEBUG 0
#define DEBUG_KQUEUE 0
#define QUEUE_ENABLE 0
/*
 * Если включить, это влияет очень сильно на CPU
 * Возможно из-за роста sockPool
 */
#define KEEP_ALIVE 0
#define RECEIVED_MESSAGE_LENGTH 2000

/**
 * This will handle connection for each client
 * @param args
 * @return
 */
void *clientTcpHandler(void *args) {
    int threadNumber = ((struct clientTcpArgs *) args)->threadNumber;
    struct rk_sema *semaphoreQueue = ((struct clientTcpArgs *) args)->semaphoreQueue;
    struct queue **queue = ((struct clientTcpArgs *) args)->queue;
    struct firstByteData *firstByteData = ((struct clientTcpArgs *) args)->firstByteData;
    struct stats *stats = ((struct clientTcpArgs *) args)->stats;

    int equeue = ((struct clientTcpArgs *) args)->equeue;

    struct rk_sema *semaphoreSocketPool = ((struct clientTcpArgs *) args)->semaphoreSocketPool;
    struct socketPool **socketPool = ((struct clientTcpArgs *) args)->socketPool;

    unsigned int *interval =  ((struct clientTcpArgs *) args)->interval;
    c_free(args);

    struct Eevent eevent;

    while (1) {
        int nev = checkEqueue(equeue, &eevent);

        DEBUG_KQUEUE && printf("thread_client_tcp.c: go nev=%d\n", nev);

        for (int index = 0; index < nev; index++) {
            int currentSocket = getSocketEqueue(&eevent, index);

            if (isEof(&eevent, index)) {
                DEBUG_KQUEUE && printf("thread_client_tcp.c: Disconnect\n");

                rk_sema_wait(semaphoreSocketPool);
                deleteSocket(socketPool, currentSocket, stats);
                rk_sema_post(semaphoreSocketPool);
                // Socket is automatically removed from the kq by the kernel.
            } else if (isRead(&eevent, index)) {
                DEBUG_KQUEUE && printf("thread_client_tcp.c: Read %d\n", currentSocket);
                // Read from socket.
                char readBuffer[RECEIVED_MESSAGE_LENGTH + 1];
                memset(readBuffer, 0, sizeof(readBuffer));
                size_t readSize = recv(currentSocket, readBuffer, sizeof(readBuffer), MSG_NOSIGNAL | MSG_PEEK);
                DEBUG_KQUEUE && printf("thread_client_tcp.c: read %zu bytes fd=%d \n", readSize, currentSocket);

                // Запрос превышает лимит, прерываю такие сокеты
                if (readSize == RECEIVED_MESSAGE_LENGTH) {
                    // printf("recv has full buffer\n");

                    struct block *block = initBlock();
                    renderHttpMessage(block, 413, "Request Entity Too Large", 24, 0, stats);
                    send_(currentSocket, block->data, block->size, stats);
                    freeBlock(block);

                    rk_sema_wait(semaphoreSocketPool);
                    deleteSocket(socketPool, currentSocket, stats);
                    rk_sema_post(semaphoreSocketPool);

                    stats->recv_failed++;

                    continue;
                }

                if (strstr(readBuffer, "\r\n\r\n") != NULL) {
                    rk_sema_wait(semaphoreSocketPool);
                    updateSocket(socketPool, currentSocket, equeue);
                    rk_sema_post(semaphoreSocketPool);

                    // Сброс буфера, поскольку запрос полный, прочитать столько сколько было пикнуто
                    recv(currentSocket, readBuffer, readSize, MSG_NOSIGNAL);


                    DEBUG_KQUEUE && printf("thread_client_tcp.c: IN %d", threadNumber);

                    DEBUG && printf("first = %p\n", queue);
                    DEBUG && printf("*first = %p\n", *queue);

                    if (QUEUE_ENABLE) {
                        rk_sema_wait(semaphoreQueue);
                        if (CHECK_SEMAPHORE) {
                            printf("Check semaphore clientTcpHandler begin = %d\n", threadNumber);
                            usleep(rand() % 5000 + 5000); // проверка работы семафора
                            printf("Check semaphore clientTcpHandler end = %d\n", threadNumber);
                        }
                        *queue = addToQueue(*queue, threadNumber);
                        rk_sema_post(semaphoreQueue);
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

                        canKeepAlive = KEEP_ALIVE && ((strstr(readBuffer, "HTTP/1.1") != NULL)
                                                      || (strstr(readBuffer, "Connection: Keep-Alive") != NULL));

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

                            if (!query.has_info_hash) {
                                renderHttpMessage(writeBlock, 400, "Field 'info_hash' must be filled", 25, canKeepAlive,
                                                  stats);
                            } else if (!query.port) {
                                renderHttpMessage(writeBlock, 400, "Field 'port' must be filled", 25, canKeepAlive,
                                                  stats);
                            } else {
                                struct torrent *torrent;
                                struct block *block = initBlock();

                                switch (query.event) {
                                    case EVENT_ID_STOPPED:
                                        waitSem(firstByteData, &query);
                                        torrent = deletePeer(firstByteData, &query);
                                        renderPeers(block, torrent, &query, interval);
                                        postSem(firstByteData, &query);
                                        break;
                                    default:
                                        waitSem(firstByteData, &query);
                                        torrent = updatePeer(firstByteData, &query);
                                        renderPeers(block, torrent, &query, interval);
                                        postSem(firstByteData, &query);
                                        break;
                                } // End of switch

                                renderHttpMessage(writeBlock, 200, block->data, block->size, canKeepAlive, stats);
                                freeBlock(block);
                            }
                        } else if (startsWith("GET /stats", readBuffer)) {
                            struct block *block = initBlock();

                            if (QUEUE_ENABLE) {
                                rk_sema_wait(semaphoreQueue);
                                printQueue(block, *queue);
                                rk_sema_post(semaphoreQueue);
                            }
                            formatStats(threadNumber, block, stats, *interval);

                            renderHttpMessage(writeBlock, 200, block->data, block->size, canKeepAlive, stats);
                            freeBlock(block);
                        } else if (startsWith("GET /garbage", readBuffer)) {
                            runGarbageCollector(firstByteData);
                            renderHttpMessage(writeBlock, 200, "OK", 2, canKeepAlive, stats);
                        } else if (startsWith("GET /scrape", readBuffer)) {
                            stats->scrape++;

                            struct query query = {};
                            struct block *hashes = initBlock();
                            struct block *block = initBlock();
                            parseUri(&query, hashes, readBuffer);

                            if (!hashes->size && !ENABLE_FULL_SCRAPE) {
                                renderHttpMessage(writeBlock, 403, "Forbidden (Full Scrape Disabled)", 32, canKeepAlive,
                                                  stats);
                            } else {
                                renderTorrents(block, firstByteData, hashes, 0);
                                renderHttpMessage(writeBlock, 200, block->data, block->size, canKeepAlive, stats);
                            }

                            freeBlock(hashes);
                            freeBlock(block);
                        } else {
                            renderHttpMessage(writeBlock, 404, "Page not found", 14, canKeepAlive, stats);
                        }

                        if (canKeepAlive) {
                            stats->keep_alive++;
                        } else {
                            stats->no_keep_alive++;
                        }
                    } // isHttp
                    else {
                        renderHttpMessage(writeBlock, 405, readBuffer, readSize, canKeepAlive, stats);
                        DEBUG && printf("< %s", readBuffer);
                    }

                    if (QUEUE_ENABLE) {
                        rk_sema_wait(semaphoreQueue);
                        *queue = deleteFromQueue(*queue, threadNumber);
                        rk_sema_post(semaphoreQueue);
                    }

                    DEBUG && printf("Recv bytes: %zu\n", readSize);
                    DEBUG_KQUEUE && printf("thread_client_tcp.c: Write %d, keep=%d\n", threadNumber, canKeepAlive);

                    send_(currentSocket, writeBlock->data, writeBlock->size, stats);
                    // Ответ дан - удаляю
                    freeBlock(writeBlock);

                    if (!canKeepAlive) {
                        rk_sema_wait(semaphoreSocketPool);
                        deleteSocket(socketPool, currentSocket, stats);
                        rk_sema_post(semaphoreSocketPool);
                        // close(currentSocket);
                    }
                } // "\r\n\r\n"
            } // EVFILT_READ
        } // for

        // Чтобы нормально работала подсветка кода в IDE
        if (rand() % 2 == 3) break;
    } // white 1

    if (pthread_detach(pthread_self()) != 0) {
        perror("Could not detach thread");
    }

    return 0;
}
