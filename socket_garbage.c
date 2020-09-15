#include <stdio.h>
#include <unistd.h>
#include "socket_garbage.h"
#include "alloc.h"
#include "socket.h"
#include "equeue.h"

#define DEBUG 0

void updateSocketL(struct list *socketList, int socket, int equeue) {
    unsigned char *pSocket = (unsigned char *) &socket;

    struct list *leaf = getLeaf(socketList, pSocket);
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

    postSemaphoreLeaf(leaf);
}

void deleteSocketItemL(struct item *item, struct stats *stats) {

    if (item == NULL) {
        printf("socket_garbage.c: socketData not found\n");

        // return;
    } else {
        struct socketData *socketData = item->data;

        int socket = socketData->socket;

        deleteClientEqueue(socketData->equeue, socket);
        int status = close(socket);
        if (status) {
            stats->close_failed++;
        } else {
            stats->close_pass++;
        }
        c_free(socketData);

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
