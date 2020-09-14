#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include "thread_garbage.h"
#include "alloc.h"
#include "data_garbage.h"
#include "stats.h"
#include "interval.h"
#include "rps.h"

#define DEBUG 0
#define I_15_MINUTES_TIME (15 * 60)
#define GARBAGE_SOCKET_POOL_TIME 1

struct i15MinutesArgs {
    struct firstByteData *firstByteData;
    unsigned int *interval;
    struct rps *rps;
};

struct garbageSocketPoolArgs {
    struct socketPool **socketPool;
    struct rk_sema *semaphoreSocketPool;
    struct stats *stats;
};

void *i15MinutesHandler(void *_args);

void *garbageSocketPoolHandler(void *_args);

void run15MinutesThread(struct firstByteData *firstByte, unsigned int *interval, struct rps *rps) {
    pthread_attr_t tattr;
    pthread_t tid;
    int ret;
    int newPriority = 5;
    struct sched_param param;

    struct i15MinutesArgs *i15MinutesArgs = c_calloc(1, sizeof(struct i15MinutesArgs));
    i15MinutesArgs->firstByteData = firstByte;
    i15MinutesArgs->interval = interval;
    i15MinutesArgs->rps = rps;

    // initialized with default attributes
    ret = pthread_attr_init(&tattr);

    // safe to get existing scheduling param
    ret = pthread_attr_getschedparam(&tattr, &param);

    if (param.sched_priority > newPriority) {
        printf("Garbage data thread change priority %d -> %d\n", param.sched_priority, newPriority);

        // set the priority; others are unchanged
        param.sched_priority = newPriority;
    } else {
        printf("Garbage data thread NO change priority %d -> %d\n", param.sched_priority, newPriority);
    }


    // setting the new scheduling param
    ret = pthread_attr_setschedparam(&tattr, &param);

    // with new priority specified
    ret = pthread_create(&tid, &tattr, i15MinutesHandler, (void *) i15MinutesArgs);
}

void *i15MinutesHandler(void *_args) {
    struct firstByteData *firstByte = ((struct i15MinutesArgs *) _args)->firstByteData;
    unsigned int *interval = ((struct i15MinutesArgs *) _args)->interval;
    struct rps *rps = ((struct i15MinutesArgs *) _args)->rps;
    c_free(_args);

    while (1) {
        struct block *block = initBlock();

        runGarbageCollector(block, firstByte);
        addStringBlock(block, "  ", 2);
        updateInterval(block, interval);

        addFormatStringBlock(block, 100, "  RPS: %.2f\n\x00", getRps(rps));
        printf("%s", block->data);

        freeBlock(block);

        sleep(I_15_MINUTES_TIME);
    }

    return 0;
}

void
runGarbageSocketPoolThread(struct socketPool **socketPool, struct rk_sema *semaphoreSocketPool, struct stats *stats) {
    pthread_attr_t tattr;
    pthread_t tid;
    int ret;
    int newPriority = 5;
    struct sched_param param;

    struct garbageSocketPoolArgs *garbageSocketPoolArgs = c_calloc(1, sizeof(struct garbageSocketPoolArgs));
    garbageSocketPoolArgs->socketPool = socketPool;
    garbageSocketPoolArgs->semaphoreSocketPool = semaphoreSocketPool;
    garbageSocketPoolArgs->stats = stats;

    // initialized with default attributes
    ret = pthread_attr_init(&tattr);

    // safe to get existing scheduling param
    ret = pthread_attr_getschedparam(&tattr, &param);

    if (param.sched_priority > newPriority) {
        printf("Garbage socket pool thread change priority %d -> %d\n", param.sched_priority, newPriority);

        // set the priority; others are unchanged
        param.sched_priority = newPriority;
    } else {
        printf("Garbage socket pool thread NO change priority %d -> %d\n", param.sched_priority, newPriority);
    }


    // setting the new scheduling param
    ret = pthread_attr_setschedparam(&tattr, &param);

    // with new priority specified
    ret = pthread_create(&tid, &tattr, garbageSocketPoolHandler, (void *) garbageSocketPoolArgs);
}

void *garbageSocketPoolHandler(void *_args) {
    struct socketPool **socketPool = ((struct garbageSocketPoolArgs *) _args)->socketPool;
    struct rk_sema *semaphoreSocketPool = ((struct garbageSocketPoolArgs *) _args)->semaphoreSocketPool;
    struct stats *stats = ((struct garbageSocketPoolArgs *) _args)->stats;
    c_free(_args);

    while (1) {
        rk_sema_wait(semaphoreSocketPool);
        unsigned int removed = runCollectSocket(socketPool, stats);
        rk_sema_post(semaphoreSocketPool);

        DEBUG && printf("thread_garbage.c: runCollectSocket: removed=%d\n", removed);

        sleep(GARBAGE_SOCKET_POOL_TIME);
    }

    return 0;
}
