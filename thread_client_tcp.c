#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include <sys/event.h>
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
#include "request.h"

#define DEBUG 0
#define DEBUG_KQUEUE 0
#define QUEUE_ENABLE 1
#define KEEP_ALIVE 1
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

    pthread_cond_t *signalRequest = ((struct clientTcpArgs *) args)->signalRequest;
    pthread_mutex_t *mutexSignalRequest = ((struct clientTcpArgs *) args)->mutexSignalRequest;
    struct request **firstRequest = ((struct clientTcpArgs *) args)->firstRequest;
    struct request **lastRequest = ((struct clientTcpArgs *) args)->lastRequest;
    struct rk_sema *semaphoreRequest = ((struct clientTcpArgs *) args)->semaphoreRequest;
    c_free(args);

    // todo delete
    sleep(1);

    while (1) {
        rk_sema_wait(semaphoreRequest);
        struct request *request = pullRequest(firstRequest, lastRequest);
        rk_sema_post(semaphoreRequest);

        if (request == NULL) {
            DEBUG_KQUEUE && printf("thread_client_tcp.c: Before signal %d\n", threadNumber);
            pthread_mutex_lock(mutexSignalRequest);
            pthread_cond_wait(signalRequest, mutexSignalRequest);
            pthread_mutex_unlock(mutexSignalRequest);
            DEBUG_KQUEUE && printf("thread_client_tcp.c: After signal %d\n", threadNumber);

            continue;
        } else {
            DEBUG_KQUEUE &&
            printf("thread_client_tcp.c: Ready request found %d, sock = %d\n", threadNumber, request->socket);
        }

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

        DEBUG && printf("Handler: sock:%d number:%d\n", request->socket, threadNumber);

        _Bool isHttp = 0;
        int canKeepAlive = 0;
        struct block *writeBlock = initBlock();

        DEBUG && printf("> %s", request->block->data);

        stats->recv_bytes += request->block->size;
        stats->recv_pass++;

        if (DEBUG && startsWith("stop", request->block->data)) {
            printf("STOP\n");
            exit(40);
        }

        if (startsWith("GET ", request->block->data)) {
            isHttp = 1;
            DEBUG && printf("isHttp = 1\n");
        }

        if (isHttp) {
            DEBUG && printf("Message complete\n");

            canKeepAlive = KEEP_ALIVE && ((strstr(request->block->data, "HTTP/1.1") != NULL)
                                          || (strstr(request->block->data, "Connection: Keep-Alive") != NULL));

            if (startsWith("GET /announce", request->block->data)) {
                stats->announce++;

                struct query query = {};

                struct sockaddr_in peer = {};
                socklen_t socklen = sizeof(peer);
                getpeername(request->socket, (struct sockaddr *) &peer, &socklen); // client
                query.ip = peer.sin_addr.s_addr;

                query.numwant = DEFAULT_NUM_WANT;
                query.event = EVENT_ID_STARTED;
                query.threadNumber = threadNumber;

                parseUri(&query, NULL, request->block->data);

                if (!query.has_info_hash) {
                    renderHttpMessage(writeBlock, 400, "Field 'info_hash' must be filled", 25, canKeepAlive, stats);
                } else if (!query.port) {
                    renderHttpMessage(writeBlock, 400, "Field 'port' must be filled", 25, canKeepAlive, stats);
                } else {
                    struct torrent *torrent;
                    struct block *block = initBlock();

                    switch (query.event) {
                        case EVENT_ID_STOPPED:
                            waitSem(firstByteData, &query);
                            torrent = deletePeer(firstByteData, &query);
                            renderPeers(block, torrent, &query);
                            postSem(firstByteData, &query);
                            break;
                        default:
                            waitSem(firstByteData, &query);
                            torrent = updatePeer(firstByteData, &query);
                            renderPeers(block, torrent, &query);
                            postSem(firstByteData, &query);
                            break;
                    } // End of switch

                    renderHttpMessage(writeBlock, 200, block->data, block->size, canKeepAlive, stats);
                    freeBlock(block);
                }
            } else if (startsWith("GET /stats", request->block->data)) {
                struct block *block = initBlock();

                if (QUEUE_ENABLE) {
                    rk_sema_wait(semaphoreQueue);
                    printQueue(block, *queue);
                    rk_sema_post(semaphoreQueue);
                }
                formatStats(threadNumber, block, stats);

                renderHttpMessage(writeBlock, 200, block->data, block->size, canKeepAlive, stats);
                freeBlock(block);
            } else if (startsWith("GET /garbage", request->block->data)) {
                runGarbageCollector(firstByteData);
                renderHttpMessage(writeBlock, 200, "OK", 2, canKeepAlive, stats);
            } else if (startsWith("GET /scrape", request->block->data)) {
                stats->scrape++;

                struct query query = {};
                struct block *hashes = initBlock();
                struct block *block = initBlock();
                parseUri(&query, hashes, request->block->data);

                if (!hashes->size && !ENABLE_FULL_SCRAPE) {
                    renderHttpMessage(writeBlock, 403, "Forbidden (Full Scrape Disabled)", 32, canKeepAlive, stats);
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
            renderHttpMessage(writeBlock, 200, request->block->data, request->block->size, canKeepAlive, stats);
            DEBUG && printf("< %s", request->block->data);
        }

        if (QUEUE_ENABLE) {
            rk_sema_wait(semaphoreQueue);
            *queue = deleteFromQueue(*queue, threadNumber);
            rk_sema_post(semaphoreQueue);
        }

        DEBUG && printf("Recv bytes: %d\n", request->block->size);
        DEBUG_KQUEUE && printf("thread_client_tcp.c: Write %d, keep=%d\n", threadNumber, canKeepAlive);

        send_(request->socket, writeBlock->data, writeBlock->size, stats);
        // Ответ дан - удаляю
        freeBlock(writeBlock);

        if (!canKeepAlive)
            close(request->socket);

        // Обработан – удаляю
        freeRequest(request);

        // Чтобы нормально работала подсветка кода в IDE
        if (rand() % 2 == 3) break;
    } // while 1

    if (pthread_detach(pthread_self()) != 0) {
        perror("Could not detach thread");
    }

    return 0;
}
