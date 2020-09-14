#include <stdio.h>
#include <unistd.h>
#include "socket_garbage.h"
#include "alloc.h"
#include "socket.h"
#include "equeue.h"

#define DEBUG 0
#define TIMEOUT 3

void updateSocket(struct socketPool **socketPool, int socket, int equeue) {
    struct socketPool *currentSocketPool = *socketPool;

    while (currentSocketPool != NULL) {
        if (currentSocketPool->socket == socket) {
            currentSocketPool->time = time(NULL);

            DEBUG && printf("socketPool #%d update\n", socket);

            return;
        }

        currentSocketPool = currentSocketPool->next;
    }

    currentSocketPool = c_calloc(1, sizeof(struct socketPool));
    currentSocketPool->socket = socket;
    currentSocketPool->time = time(NULL);
    currentSocketPool->next = *socketPool;
    currentSocketPool->equeue = equeue;

    *socketPool = currentSocketPool;

    DEBUG && printf("socketPool #%d new\n", socket);
}

void deleteSocket(struct socketPool **socketPool, int socket, struct stats *stats) {
    struct socketPool *currentSocketPool = *socketPool;
    struct socketPool *previous = NULL;

    while (currentSocketPool != NULL) {
        if (currentSocketPool->socket == socket) {
            if (previous == NULL) {
                *socketPool = currentSocketPool->next;

            } else {
                previous->next = currentSocketPool->next;
            }

            deleteClientEqueue(currentSocketPool->equeue, currentSocketPool->socket);
            int status = close(currentSocketPool->socket);
            if (status) {
                stats->close_failed++;
            } else {
                stats->close_pass++;
            }
            DEBUG && printf("Close socket=%d, status=%d\n", currentSocketPool->socket, status);
            c_free(currentSocketPool);

            DEBUG && printf("socketPool #%d delete\n", socket);

            return;
        }

        previous = currentSocketPool;
        currentSocketPool = currentSocketPool->next;
    }

    printf("Socket #%d not found for delete\n", socket);
}

unsigned int runCollectSocket(struct socketPool **socketPool, struct stats *stats) {
    struct socketPool *currentSocketPool = *socketPool;
    struct socketPool *previous = NULL;
    long diffTime = time(NULL) - TIMEOUT;

    unsigned int removed = 0;

    while (currentSocketPool != NULL) {
        if (currentSocketPool->time <= diffTime) {
            if (previous == NULL) {
                *socketPool = currentSocketPool->next;
            } else {
                previous->next = currentSocketPool->next;
            }

            struct socketPool *delete = currentSocketPool;
            currentSocketPool = currentSocketPool->next;

            struct block *block = initBlock();
            renderHttpMessage(block, 408, "Request Timeout", 15, 0, stats);
            send_(delete->socket, block->data, block->size, stats);
            freeBlock(block);


            deleteClientEqueue(delete->equeue, delete->socket);
            int status = close(delete->socket);
            if (status) {
                stats->close_failed++;
            } else {
                stats->close_pass++;
            }

            DEBUG && printf("Close socket=%d, status=%d\n", delete->socket, status);
            DEBUG && printf("socketPool #%d collect\n", delete->socket);
            c_free(delete);

            removed++;

            continue;
        }

        previous = currentSocketPool;
        currentSocketPool = currentSocketPool->next;
    }

    return removed;
}
