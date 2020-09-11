#include <stdio.h>
#include <unistd.h>
#include "socket_garbage.h"
#include "alloc.h"

#define DEBUG 0
#define TIMEOUT 3

void updateSocket(struct socketPool **socketPool, int socket) {
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

    *socketPool = currentSocketPool;

    DEBUG && printf("socketPool #%d new\n", socket);
}

void deleteSocket(struct socketPool **socketPool, int socket) {
    struct socketPool *currentSocketPool = *socketPool;
    struct socketPool *previous = NULL;

    while (currentSocketPool != NULL) {
        if (currentSocketPool->socket == socket) {
            if (previous == NULL) {
                *socketPool = currentSocketPool->next;

            } else {
                previous->next = currentSocketPool->next;
            }

            close(currentSocketPool->socket);
            c_free(currentSocketPool);

            DEBUG && printf("socketPool #%d delete\n", socket);

            return;
        }

        previous = currentSocketPool;
        currentSocketPool = currentSocketPool->next;
    }

    printf("Socket #%d not found for delete\n", socket);
}

unsigned int runCollectSocket(struct socketPool **socketPool) {
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

            DEBUG && printf("socketPool #%d collect\n", currentSocketPool->socket);

            struct socketPool *delete = currentSocketPool;
            currentSocketPool = currentSocketPool->next;

            close(delete->socket);
            c_free(delete);

            removed++;

            continue;
        }

        previous = currentSocketPool;
        currentSocketPool = currentSocketPool->next;
    }

    return removed;
}
