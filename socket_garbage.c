#include <stdio.h>
#include <unistd.h>
#include "socket_garbage.h"
#include "alloc.h"
#include "socket.h"
#include "equeue.h"

void updateSocketL(struct list *socketList, int socket, int equeue, _Bool useSemaphore) {
    unsigned char *pSocket = (unsigned char *) &socket;

    struct list *leaf = getLeaf(socketList, pSocket);
    if (useSemaphore)
        waitSemaphoreLeaf(leaf);

    struct item *item = setHash(socketList, pSocket);

    struct socketData *socketData;
    if (item->data == NULL) {
        socketData = c_calloc(1, sizeof(struct socketData));
    } else {
        socketData = item->data;
    }
    socketData->socket = socket;
    socketData->equeue = equeue;
    socketData->time = time(NULL);

    item->data = socketData;

    if (useSemaphore)
        postSemaphoreLeaf(leaf);
}

void deleteSocketItemL(struct item *item, struct stats *stats) {
    if (item == NULL) {
        long now = time(NULL);

        printf("%.19s socket_garbage.c: socketData not found\n", ctime((time_t *) &now));
    } else {
        struct socketData *socketData = item->data;

        int socket = socketData->socket;

        deleteClientEqueue(socketData->equeue, socket);
        int status = close(socket);
        if (status) {
            incErrno(stats->close_errno);
            stats->close_failed++;
        } else {
            stats->close_pass++;
        }

        deleteItem(item);
    }
}

void deleteSocketL(struct list *socketList, int socket, struct stats *stats) {
    unsigned char *pSocket = (unsigned char *) &socket;

    struct list *leaf = getLeaf(socketList, pSocket);
    waitSemaphoreLeaf(leaf);

    struct item *item = getHash(socketList, pSocket);
    deleteSocketItemL(item, stats);

    postSemaphoreLeaf(leaf);
}
