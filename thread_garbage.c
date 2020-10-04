#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdatomic.h>
#include "thread_garbage.h"
#include "alloc.h"
#include "data_garbage.h"
#include "stats.h"
#include "interval.h"
#include "rps.h"
#include "list.h"
#include "socket.h"
#include "block.h"
#include "thread.h"

#define DEBUG 0
#define GARBAGE_SOCKET_POOL_TIME 1

struct i15MinutesArgs {
    struct list *torrentList;
    _Atomic(unsigned int) *interval;
    struct rps *rps;
    unsigned int minInterval;
    unsigned int maxInterval;
};

struct garbageSocketTimeoutArgs {
    struct list **socketLists;
    struct stats *stats;
    unsigned short *socketTimeout;
    long workers;
    long maxTimeAllow; // Временная переменная, чтобы не считать это значение для каждого подключения
};

void *i15MinutesHandler(struct i15MinutesArgs *args);

void *garbageSocketTimeoutHandler(struct garbageSocketTimeoutArgs *_args);

void run15MinutesThread(struct list *torrentList, _Atomic(unsigned int) *interval, struct rps *rps,
                        unsigned int minInterval, unsigned int maxInterval) {
    struct i15MinutesArgs *i15MinutesArgs = c_calloc(1, sizeof(struct i15MinutesArgs));
    i15MinutesArgs->torrentList = torrentList;
    i15MinutesArgs->interval = interval;
    i15MinutesArgs->rps = rps;
    i15MinutesArgs->minInterval = minInterval;
    i15MinutesArgs->maxInterval = maxInterval;

    pthread_t tid;
    pthread_create(&tid, NULL, (void *(*)(void *)) i15MinutesHandler, i15MinutesArgs);
}

void *i15MinutesHandler(struct i15MinutesArgs *args) {
    pthreadSetName(pthread_self(), "Garbage i15");

    struct list *torrentList = args->torrentList;
    _Atomic(unsigned int) *interval = args->interval;
    struct rps *rps = args->rps;
    unsigned int minInterval = args->minInterval;
    unsigned int maxInterval = args->maxInterval;
    c_free(args);

    while (1) {
        struct block *block = initBlock();

        runGarbageCollectorL(block, torrentList);
        addStringBlock(block, "  ", 2);
        updateInterval(block, interval, minInterval, maxInterval);

        addFormatStringBlock(block, 100, "  RPS: %.2f\n\x00", getRps(rps));
        printf("%s", block->data);

        freeBlock(block);

        sleep(*interval);

        // Чтобы нормально работала подсветка кода в IDE
        if (rand() % 2 == 3) break;
    }

    return 0;
}

void runGarbageSocketTimeoutThread(struct list **socketLists, struct stats *stats, unsigned short *socketTimeout, long workers) {
    struct garbageSocketTimeoutArgs *garbageSocketTimeoutArgs = c_calloc(1, sizeof(struct garbageSocketTimeoutArgs));
    garbageSocketTimeoutArgs->socketLists = socketLists;
    garbageSocketTimeoutArgs->stats = stats;
    garbageSocketTimeoutArgs->socketTimeout = socketTimeout;
    garbageSocketTimeoutArgs->workers = workers;

    pthread_t tid;
    pthread_create(&tid, NULL, (void *(*)(void *)) garbageSocketTimeoutHandler, garbageSocketTimeoutArgs);
}

unsigned char garbageSocketTimeoutCallback(struct list *list, struct item *item, void *args) {
    // unused
    if (list == NULL) {
        printf("thread_garbage.c: unused list\n");

        exit(123);
    }

    struct socketData *socketData = item->data;
    struct stats *stats = ((struct garbageSocketTimeoutArgs *) args)->stats;
    long maxTimeAllow = ((struct garbageSocketTimeoutArgs *) args)->maxTimeAllow;

    // printf("%ld < %ld\n", socketData->time, maxTimeAllow);

    if (socketData->time < maxTimeAllow) {
        struct block *block = initBlock();
        renderHttpMessage(block, 408, "Request Timeout", 15, 0, 0, stats, NULL, NULL);
        send_(socketData->socket, block->data, block->size, stats);
        freeBlock(block);

        deleteSocketItemL(item, stats);
    }

    return 0;
}

void *garbageSocketTimeoutHandler(struct garbageSocketTimeoutArgs *_args) {
    pthreadSetName(pthread_self(), "Garbage socket");

    struct list **socketLists = _args->socketLists;
    long workers = _args->workers;

    while (1) {
        for (int workerNumber = 0; workerNumber < workers; ++workerNumber) {
            struct list *socketList = socketLists[workerNumber];

            waitSemaphoreLeaf(socketList);

            unsigned short *socketTimeout = _args->socketTimeout;
            /*
             * 1 – это одна секунда дополнительного времени, который сборщик таймаут подключений даёт клиенту,
             * чтобы тот сам закрыл подключение, которое он считает ненужным (по заголоаку Keep-Alive)
             */
            _args->maxTimeAllow = time(NULL) - *socketTimeout - 1;

            mapList(socketList, _args, &garbageSocketTimeoutCallback);
            postSemaphoreLeaf(socketList);
        }

        sleep(GARBAGE_SOCKET_POOL_TIME);

        // Чтобы нормально работала подсветка кода в IDE
        if (rand() % 2 == 3) break;
    }

    c_free(_args);

    return 0;
}
