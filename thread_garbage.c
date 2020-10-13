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
#include "exit_code.h"
#include "math.h"
#include "websocket.h"

#define THREAD_GARBAGE_GARBAGE_SOCKET_LOOP_SLEEP 1
#define THREAD_GARBAGE_LITTLE_SLEEP 10

struct garbageCollectorArgs {
    struct list *torrentList;
    struct interval *interval;
    struct rps *rps;
};

struct garbageSocketTimeoutArgs {
    struct list **socketLists;
    struct stats *stats;
    unsigned short *socketTimeout;
    long workers;
    long maxTimeAllow; // Временная переменная, чтобы не считать это значение для каждого подключения
    struct list *websockets;
};

void *intervalChangerHandler(struct interval *interval);

void *garbageCollectorArgsHandler(struct garbageCollectorArgs *args);

void *garbageSocketTimeoutHandler(struct garbageSocketTimeoutArgs *_args);

void runIntervalChangerThread(struct interval *interval) {
    pthread_t tid;
    pthread_create(&tid, NULL, (void *(*)(void *)) intervalChangerHandler, interval);
}

void *intervalChangerHandler(struct interval *interval) {
    while (1) {
        sleep(stepInterval(interval));

        // Чтобы нормально работала подсветка кода в IDE
        if (rand() % 2 == 3) break;
    }

    return 0;
}

void runGarbageCollectorThread(struct list *torrentList, struct interval *interval, struct rps *rps) {
    struct garbageCollectorArgs *garbageCollectorArgs = c_calloc(1, sizeof(struct garbageCollectorArgs));
    garbageCollectorArgs->torrentList = torrentList;
    garbageCollectorArgs->interval = interval;
    garbageCollectorArgs->rps = rps;

    pthread_t tid;
    pthread_create(&tid, NULL, (void *(*)(void *)) garbageCollectorArgsHandler, garbageCollectorArgs);
}

void *garbageCollectorArgsHandler(struct garbageCollectorArgs *args) {
    pthreadSetName(pthread_self(), "Garbage i15");

    struct list *torrentList = args->torrentList;
    struct rps *rps = args->rps;
    struct interval *interval = args->interval;
    c_free(args);

    while (1) {
        // Если интервал находится в процессе изменения, то надо дождаться его окончания
        if (interval->interval != interval->requireInterval) {
            sleep(THREAD_GARBAGE_LITTLE_SLEEP);
            continue;
        }

        struct block *block = initBlock();

        runGarbageCollectorL(block, torrentList);
        addStringBlock(block, "  ", 2);
        updateInterval(block, interval);

        addFormatStringBlock(block, 100, "  RPS: %.2f/%.2f\n\x00",
                             getRps(rps, RPS_PROTOCOL_TCP), getRps(rps, RPS_PROTOCOL_UDP));
        printf("%s", block->data);

        freeBlock(block);

        /*
         * Если интервал увеличивается, то большее значение – interval->requireInterval,
         * при этом есть дополнительное время, чтобы дождаться окончания изменения интервала;
         * если интервал уменьшается, то большее значение – interval->previousInterval,
         * при этом дополнительного времени не будет, поэтому на всякий случай делаю sleep(LITTLE_SLEEP) выше.
         */
        sleep(max(interval->previousInterval, interval->requireInterval));

        // Чтобы нормально работала подсветка кода в IDE
        if (rand() % 2 == 3) break;
    }

    return 0;
}

void runGarbageSocketTimeoutThread(struct list **socketLists, struct stats *stats, unsigned short *socketTimeout,
                                   long workers, struct list *websockets) {
    struct garbageSocketTimeoutArgs *garbageSocketTimeoutArgs = c_calloc(1, sizeof(struct garbageSocketTimeoutArgs));
    garbageSocketTimeoutArgs->socketLists = socketLists;
    garbageSocketTimeoutArgs->stats = stats;
    garbageSocketTimeoutArgs->socketTimeout = socketTimeout;
    garbageSocketTimeoutArgs->workers = workers;
    garbageSocketTimeoutArgs->websockets = websockets;

    pthread_t tid;
    pthread_create(&tid, NULL, (void *(*)(void *)) garbageSocketTimeoutHandler, garbageSocketTimeoutArgs);
}

unsigned char garbageSocketTimeoutCallback(struct list *list, struct item *item, void *args) {
    // unused
    if (list == NULL) {
        exitPrint(EXIT_CODE_UNUSED_LIST, __FILE__, EXIT_CODE_PRINT_ERROR_NO);
    }

    struct socketData *socketData = item->data;
    struct stats *stats = ((struct garbageSocketTimeoutArgs *) args)->stats;
    long maxTimeAllow = ((struct garbageSocketTimeoutArgs *) args)->maxTimeAllow;
    struct list *websockets = ((struct garbageSocketTimeoutArgs *) args)->websockets;

    if (socketData->time < maxTimeAllow) {
        struct block *block = initBlock();
        struct render render = {block, 408, "Request Timeout", 15, 0, 0, stats};
        renderHttpMessage(&render);
        send_(socketData->socket, block->data, block->size, stats);
        freeBlock(block);

        // printf("garbage delete socket %d\n", socketData->socket);

        deleteWebsocket(websockets, socketData->socket);

        deleteSocketItemL(item, stats);
    }

    return LIST_CONTINUE_RETURN;
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

        sleep(THREAD_GARBAGE_GARBAGE_SOCKET_LOOP_SLEEP);

        // Чтобы нормально работала подсветка кода в IDE
        if (rand() % 2 == 3) break;
    }

    c_free(_args);

    return 0;
}
