#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <unistd.h>
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

#define DEBUG 0
#define QUEUE_ENABLE 0
#define KEEP_ALIVE 1
#define RECEIVED_MESSAGE_LENGTH 2000

/**
 * This will handle connection for each client
 * @param args
 * @return
 */
void *clientTcpHandler(void *args) {
    int threadSocket = ((struct clientTcpArgs *) args)->sock;
    int threadNumber = ((struct clientTcpArgs *) args)->number;
    struct rk_sema *sem = ((struct clientTcpArgs *) args)->sem;
    struct queue **first = ((struct clientTcpArgs *) args)->first;
    in_addr_t ip = ((struct clientTcpArgs *) args)->ip;
    struct firstByte *firstByte = ((struct clientTcpArgs *) args)->firstByte;
    struct stats *stats = ((struct clientTcpArgs *) args)->stats;
    c_free(args);

    DEBUG && printf("first = %p\n", first);
    DEBUG && printf("*first = %p\n", *first);

    if (QUEUE_ENABLE) {
        rk_sema_wait(sem);
        if (CHECK_SEMAPHORE) {
            printf("Check semaphore clientTcpHandler begin = %d\n", threadNumber);
            usleep(rand() % 5000 + 5000); // проверка работы семафора
            printf("Check semaphore clientTcpHandler end = %d\n", threadNumber);
        }
        *first = addToQueue(*first, threadNumber);
        rk_sema_post(sem);
    }

    DEBUG && printf("Handler: sock:%d number:%d\n", threadSocket, threadNumber);

    int receivedSize;
    _Bool isHttp = 0;
    struct block *fullMessage = initBlock();
    char receivedMessage[RECEIVED_MESSAGE_LENGTH + 1] = {0};

    _Bool onceReceiveSuccess = 0; // Однажды это соединение удачно выполнило фукнцию recv

    while (memset(receivedMessage, 0, sizeof(receivedMessage))
           && (receivedSize = recv(threadSocket, receivedMessage, RECEIVED_MESSAGE_LENGTH, MSG_NOSIGNAL)) > 0) {
        DEBUG && printf("> %s", receivedMessage);

        onceReceiveSuccess = 1;

        stats->recv_bytes += receivedSize;
        stats->recv_pass++;

        if (startsWith("stop", receivedMessage)) {
            printf("STOP\n");
            exit(40);
        }

        if (!isHttp && startsWith("GET ", receivedMessage)) {
            isHttp = 1;
            DEBUG && printf("isHttp = 1\n");
        }

        if (isHttp) {
            addStringBlock(fullMessage, receivedMessage, receivedSize);
            DEBUG && printf("message = %s", fullMessage->data);

            if (strstr(fullMessage->data, "\r\n\r\n") != NULL) {
                DEBUG && printf("Message complete\n");

                int canKeepAlive = KEEP_ALIVE && ((strstr(fullMessage->data, "HTTP/1.1") != NULL)
                                                  || (strstr(fullMessage->data, "Connection: Keep-Alive") != NULL));

                if (startsWith("GET /announce", fullMessage->data)) {
                    struct query query = {0};
                    query.ip = ip;
                    query.numwant = DEFAULT_NUM_WANT;
                    query.event = EVENT_ID_STARTED;
                    query.threadNumber = threadNumber;

                    parseUri(&query, NULL, fullMessage->data);

                    if (!query.has_info_hash) {
                        sendMessage(threadSocket, 400, "Field 'info_hash' must be filled", 25, canKeepAlive, stats);
                    } else if (!query.port) {
                        sendMessage(threadSocket, 400, "Field 'port' must be filled", 25, canKeepAlive, stats);
                    } else {
                        struct torrent *torrent;
                        struct block *block = initBlock();

                        switch (query.event) {
                            case EVENT_ID_STOPPED:
                                waitSem(firstByte, &query);
                                torrent = deletePeer(firstByte, &query);
                                renderPeers(block, torrent, &query);
                                postSem(firstByte, &query);
                                break;
                            default:
                                waitSem(firstByte, &query);
                                torrent = updatePeer(firstByte, &query);
                                renderPeers(block, torrent, &query);
                                postSem(firstByte, &query);
                                break;
                        } // End of switch

                        sendMessage(threadSocket, 200, block->data, block->size, canKeepAlive, stats);
                        freeBlock(block);
                    }
                } else if (startsWith("GET /stats", fullMessage->data)) {
                    struct block *block = initBlock();

                    if (QUEUE_ENABLE) {
                        rk_sema_wait(sem);
                        printQueue(block, *first);
                        rk_sema_post(sem);
                    }
                    formatStats(threadNumber, block, stats);

                    sendMessage(threadSocket, 200, block->data, block->size, canKeepAlive, stats);
                    freeBlock(block);
                } else if (startsWith("GET /garbage", fullMessage->data)) {
                    runGarbageCollector(firstByte);
                    sendMessage(threadSocket, 200, "OK", 2, canKeepAlive, stats);
                } else if (startsWith("GET /scrape", fullMessage->data)) {
                    struct query query = {0};
                    struct block *hashes = initBlock();
                    struct block *block = initBlock();
                    parseUri(&query, hashes, fullMessage->data);

                    if (!hashes->size && !ENABLE_FULL_SCRAPE) {
                        sendMessage(threadSocket, 403, "Forbidden (Full Scrape Disabled)", 32, canKeepAlive, stats);
                    } else {
                        renderTorrents(block, firstByte, hashes, 0);
                        sendMessage(threadSocket, 200, block->data, block->size, canKeepAlive, stats);
                    }

                    freeBlock(hashes);
                    freeBlock(block);
                } else {
                    sendMessage(threadSocket, 404, "Page not found", 14, canKeepAlive, stats);
                }

                fullMessage = resetBlock(fullMessage);

                if (canKeepAlive) {
                    stats->keep_alive++;
                    continue; // Connection: Keep-Alive
                } else {
                    stats->no_keep_alive++;
                    break;
                }
            } // fullMessage

            continue;
        } // isHttp


        send_(threadSocket, receivedMessage, receivedSize, stats);
        DEBUG && printf("< %s", receivedMessage);
    }

    freeBlock(fullMessage);

    close(threadSocket);

    if (QUEUE_ENABLE) {
        rk_sema_wait(sem);
        *first = deleteFromQueue(*first, threadNumber);
        rk_sema_post(sem);
    }

    DEBUG && printf("Recv bytes: %d\n", receivedSize);

    if (receivedSize == 0) {
        DEBUG && puts("Client Disconnected");
    } else if (receivedSize < 0) {
        stats->recv_failed++;
        if (onceReceiveSuccess) {
            stats->recv_failed_after_success++;
        }
        if (DEBUG) perror("Recv failed");
    } else {
        DEBUG && puts("I Disconnect Client");
    }

    if (pthread_detach(pthread_self()) != 0) {
        perror("Could not detach thread");
    }

    return 0;
}
