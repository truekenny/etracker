#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sched.h>
#include <unistd.h>
#include "thread.h"
#include "sem.h"
#include "alloc.h"
#include "queue.h"
#include "string.h"
#include "uri.h"
#include "data.h"
#include "socket.h"
#include "block.h"

#define DEBUG 0
#define QUEUE_ENABLE 0
#define KEEP_ALIVE 1
#define RECEIVED_MESSAGE_LENGTH 2000
#define GARBAGE_COLLECTOR_TIME (15 * 60)

struct garbageCollectorArgs {
    struct firstByte *firstByte;
};

void *garbageCollectorThread(void *_args);

void runGarbageCollectorThread(struct firstByte *firstByte) {
    pthread_attr_t tattr;
    pthread_t tid;
    int ret;
    int newPriority = 5;
    struct sched_param param;

    struct garbageCollectorArgs *garbageCollectorArgs = c_calloc(1, sizeof(struct garbageCollectorArgs));
    garbageCollectorArgs->firstByte = firstByte;

    // initialized with default attributes
    ret = pthread_attr_init(&tattr);

    // safe to get existing scheduling param
    ret = pthread_attr_getschedparam(&tattr, &param);

    if (param.sched_priority > newPriority) {
        printf("Garbage thread change priority %d -> %d\n", param.sched_priority, newPriority);

        // set the priority; others are unchanged
        param.sched_priority = newPriority;
    } else {
        printf("Garbage thread NO change priority %d -> %d\n", param.sched_priority, newPriority);
    }


    // setting the new scheduling param
    ret = pthread_attr_setschedparam(&tattr, &param);

    // with new priority specified
    ret = pthread_create(&tid, &tattr, garbageCollectorThread, (void *) garbageCollectorArgs);
}

void *garbageCollectorThread(void *_args) {
    struct firstByte *firstByte = ((struct garbageCollectorArgs *) _args)->firstByte;
    c_free(_args);

    while (1) {
        runGarbageCollector(firstByte);
        sleep(GARBAGE_COLLECTOR_TIME);
    }

    return 0;
}

/**
 * This will handle connection for each client
 * @param _args
 * @return
 */
void *connection_handler(void *_args) {
    int threadSocket = ((struct args *) _args)->sock;
    int threadNumber = ((struct args *) _args)->number;
    struct rk_sema *sem = ((struct args *) _args)->sem;
    struct queue **first = ((struct args *) _args)->first;
    in_addr_t ip = ((struct args *) _args)->ip;
    struct firstByte *firstByte = ((struct args *) _args)->firstByte;
    struct stats *stats = ((struct args *) _args)->stats;

    DEBUG && printf("first = %p\n", first);
    DEBUG && printf("*first = %p\n", *first);

    c_free(_args);

    if (QUEUE_ENABLE) {
        rk_sema_wait(sem);
        if (CHECK_SEMAPHORE) {
            printf("Check semaphore connection_handler begin = %d\n", threadNumber);
            usleep(rand() % 5000 + 5000); // проверка работы семафора
            printf("Check semaphore connection_handler end = %d\n", threadNumber);
        }
        *first = addToQueue(*first, threadNumber);
        rk_sema_post(sem);
    }

    DEBUG && printf("Handler: sock:%d number:%d\n", threadSocket, threadNumber);

    int receivedSize;
    _Bool isHttp = 0;
    struct block *fullMessage = initBlock();
    char receivedMessage[RECEIVED_MESSAGE_LENGTH + 1] = {0};

    while (memset(receivedMessage, 0, sizeof(receivedMessage))
           && (receivedSize = recv(threadSocket, receivedMessage, RECEIVED_MESSAGE_LENGTH, MSG_NOSIGNAL)) > 0) {
        DEBUG && printf("> %s", receivedMessage);

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
                    struct block *block = {0};

                    if (QUEUE_ENABLE) {
                        rk_sema_wait(sem);
                        block = printQueue(*first);
                        rk_sema_post(sem);
                    } else {
                        block = initBlock();
                        addFormatStringBlock(block, 2500,
                                             "Stats Offline:\n"
                                             "start_time = %.24s\n" "thread_number = %d\n"
                                             "stats.http_200 = %d\n" "stats.http_400 = %d\n"
                                             "stats.http_403 = %d\n" "stats.http_404 = %d\n"
                                             "stats.send_pass = %d\n" "stats.recv_pass = %d\n"
                                             "stats.accept_pass = %d\n"
                                             "stats.send_failed = %d\n" "stats.recv_failed = %d\n"
                                             "stats.accept_failed = %d\n"
                                             "stats.keep_alive = %d\n" "stats.no_keep_alive = %d\n"
                                             "stats.sent_bytes = %llu\n" "stats.recv_bytes = %llu\n",
                                             ctime(&stats->time), threadNumber,
                                             stats->http_200, stats->http_400,
                                             stats->http_403, stats->http_404,
                                             stats->send_pass, stats->recv_pass,
                                             stats->accept_pass,
                                             stats->send_failed, stats->recv_failed,
                                             stats->accept_failed,
                                             stats->keep_alive, stats->no_keep_alive,
                                             stats->sent_bytes, stats->recv_bytes
                        );
                    }

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
                        renderTorrents(block, firstByte, hashes);
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
        if (DEBUG) perror("Recv failed");
    } else {
        DEBUG && puts("I Disconnect Client");
    }

    if (pthread_detach(pthread_self()) != 0) {
        perror("Could not detach thread");
    }

    return 0;
}
