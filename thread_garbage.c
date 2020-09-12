#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include "thread_garbage.h"
#include "alloc.h"
#include "data_garbage.h"
#include "stats.h"

#define DEBUG 0
#define GARBAGE_DATA_TIME (15 * 60)
#define GARBAGE_SOCKET_POOL_TIME 1

struct garbageDataArgs {
    struct firstByteData *firstByte;
};

struct garbageSocketPoolArgs {
    struct socketPool **socketPool;
    struct rk_sema *semaphoreSocketPool;
    struct stats *stats;
};

void *garbageDataHandler(void *_args);

void *garbageSocketPoolHandler(void *_args);

void runGarbageDataThread(struct firstByteData *firstByte) {
    pthread_attr_t tattr;
    pthread_t tid;
    int ret;
    int newPriority = 5;
    struct sched_param param;

    struct garbageDataArgs *garbageDataArgs = c_calloc(1, sizeof(struct garbageDataArgs));
    garbageDataArgs->firstByte = firstByte;

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
    ret = pthread_create(&tid, &tattr, garbageDataHandler, (void *) garbageDataArgs);
}

void *garbageDataHandler(void *_args) {
    struct firstByteData *firstByte = ((struct garbageDataArgs *) _args)->firstByte;
    c_free(_args);

    while (1) {
        runGarbageCollector(firstByte);
        sleep(GARBAGE_DATA_TIME);
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
