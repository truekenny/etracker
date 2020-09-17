#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "queue.h"
#include "alloc.h"

#define DEBUG 0

void addQueueList(struct list *list, int threadNumber) {
    unsigned char *pThreadNumber = (unsigned char *) &threadNumber;

    struct list *leaf = getLeaf(list, pThreadNumber);
    waitSemaphoreLeaf(leaf);

    setHash(list, (unsigned char *) &threadNumber);

    postSemaphoreLeaf(leaf);
}

void deleteQueueList(struct list *list, int threadNumber) {
    unsigned char *pThreadNumber = (unsigned char *) &threadNumber;

    struct list *leaf = getLeaf(list, pThreadNumber);
    waitSemaphoreLeaf(leaf);

    deleteHash(list, (unsigned char *) &threadNumber);

    postSemaphoreLeaf(leaf);
}

unsigned char printQueueListCallback(struct list *list, struct item *item, void *args) {
    if (list == NULL) {
        // error: unused parameter 'list' [-Werror,-Wunused-parameter]
    }

    struct block *block = args;

    int threadNumber = *(int *) item->hash;

    addFormatStringBlock(block, 50, "queueList: %d\n", threadNumber);

    return 0;
}

void printQueueList(struct block *block, struct list *queueList) {
    mapList(queueList, block, &printQueueListCallback);

    addStringBlock(block, "\n", 1);
}
