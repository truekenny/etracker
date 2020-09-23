#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "thread_garbage.h"
#include "alloc.h"
#include "data_garbage.h"
#include "stats.h"
#include "interval.h"
#include "rps.h"
#include "list.h"
#include "socket.h"
#include "block.h"

#define DEBUG 0
#define GARBAGE_SOCKET_POOL_TIME 1

struct i15MinutesArgs {
    struct list *torrentList;
    unsigned int *interval;
    struct rps *rps;
};

struct garbageSocketTimeoutArgs {
    struct list *socketList;
    struct stats *stats;
    unsigned short *socketTimeout;
    long maxTimeAllow; // Временная переменная, чтобы не считать это значение для каждого подключения
};

void *i15MinutesHandler(void *_args);

void *garbageSocketTimeoutHandler(void *_args);

void run15MinutesThread(struct list *torrentList, unsigned int *interval, struct rps *rps) {
    pthread_attr_t tattr;
    pthread_t tid;
    int ret;
    int newPriority = 5;
    struct sched_param param;

    struct i15MinutesArgs *i15MinutesArgs = c_calloc(1, sizeof(struct i15MinutesArgs));
    i15MinutesArgs->torrentList = torrentList;
    i15MinutesArgs->interval = interval;
    i15MinutesArgs->rps = rps;

    ret = pthread_attr_init(&tattr);
    ret = pthread_attr_getschedparam(&tattr, &param);

    if (param.sched_priority > newPriority) {
        printf("Garbage data thread change priority %d -> %d\n", param.sched_priority, newPriority);
        param.sched_priority = newPriority;
    } else {
        printf("Garbage data thread NO change priority %d -> %d\n", param.sched_priority, newPriority);
    }

    ret = pthread_attr_setschedparam(&tattr, &param);
    ret = pthread_create(&tid, &tattr, i15MinutesHandler, (void *) i15MinutesArgs);
}

void *i15MinutesHandler(void *_args) {
    struct list *torrentList = ((struct i15MinutesArgs *) _args)->torrentList;
    unsigned int *interval = ((struct i15MinutesArgs *) _args)->interval;
    struct rps *rps = ((struct i15MinutesArgs *) _args)->rps;
    c_free(_args);

    while (1) {
        struct block *block = initBlock();

        runGarbageCollectorL(block, torrentList);
        addStringBlock(block, "  ", 2);
        updateInterval(block, interval);

        addFormatStringBlock(block, 100, "  RPS: %.2f\n\x00", getRps(rps));
        printf("%s", block->data);

        freeBlock(block);

        sleep(*interval);

        // Чтобы нормально работала подсветка кода в IDE
        if (rand() % 2 == 3) break;
    }

    return 0;
}

void runGarbageSocketTimeoutThread(struct list *socketList, struct stats *stats, unsigned short *socketTimeout) {
    pthread_attr_t tattr;
    pthread_t tid;
    int ret;
    int newPriority = 5;
    struct sched_param param;

    struct garbageSocketTimeoutArgs *garbageSocketTimeoutArgs = c_calloc(1, sizeof(struct garbageSocketTimeoutArgs));
    garbageSocketTimeoutArgs->socketList = socketList;
    garbageSocketTimeoutArgs->stats = stats;
    garbageSocketTimeoutArgs->socketTimeout = socketTimeout;

    ret = pthread_attr_init(&tattr);
    ret = pthread_attr_getschedparam(&tattr, &param);

    if (param.sched_priority > newPriority) {
        printf("Garbage socket pool thread change priority %d -> %d\n", param.sched_priority, newPriority);
        param.sched_priority = newPriority;
    } else {
        printf("Garbage socket pool thread NO change priority %d -> %d\n", param.sched_priority, newPriority);
    }

    ret = pthread_attr_setschedparam(&tattr, &param);
    ret = pthread_create(&tid, &tattr, garbageSocketTimeoutHandler, (void *) garbageSocketTimeoutArgs);
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
        renderHttpMessage(block, 408, "Request Timeout", 15, 0, 0, stats);
        send_(socketData->socket, block->data, block->size, stats);
        freeBlock(block);

        deleteSocketItemL(item, stats);
    }

    return 0;
}

void *garbageSocketTimeoutHandler(void *_args) {
    struct list *socketList = ((struct garbageSocketTimeoutArgs *) _args)->socketList;

    while (1) {
        unsigned short *socketTimeout = ((struct garbageSocketTimeoutArgs *) _args)->socketTimeout;
        ((struct garbageSocketTimeoutArgs *) _args)->maxTimeAllow = time(NULL) - *socketTimeout;
        mapList(socketList, _args, &garbageSocketTimeoutCallback);

        sleep(GARBAGE_SOCKET_POOL_TIME);

        // Чтобы нормально работала подсветка кода в IDE
        if (rand() % 2 == 3) break;
    }

    c_free(_args);

    return 0;
}
