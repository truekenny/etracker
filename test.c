#include <stdio.h>
#include "queue.h"
#include "block.h"

struct queue *first = NULL;

int testQueue();

int testBlock();

int main() {
    printf("testQueue complete = %d\n", testQueue());
    printf("testBlock complete = %d\n", testBlock());
}

int testBlock() {
    struct block *block = initBlock();

    addStringBlock(block, "12\00045", 5);
    addStringBlock(block, "12345", 5);
    addStringBlock(block, "12345", 5);
    addStringBlock(block, "12345", 5);
    addFormatStringBlock(block, 10, "%d", 67890);
    addFormatStringBlock(block, 10, "%d", 67890);
    addFormatStringBlock(block, 10, "%d", 67890);
    addFormatStringBlock(block, 10, "%d", 67890);

    printf("12_4512345123451234567890678906789067890\n");

    for (int i = 0; i < block->size; i++) {
        if (block->data[i] == 0) {
            printf("_");

            continue;
        }

        printf("%c", block->data[i]);
    }
    printf("\n");

    return 1;
}

int testQueue() {
    first = addToQueue(first, 1);
    first = addToQueue(first, 2);
    first = addToQueue(first, 3);
    first = addToQueue(first, 4);
    printQueue(first);

    first = deleteFromQueue(first, 2);
    printQueue(first);

    first = deleteFromQueue(first, 1);
    printQueue(first);

    first = deleteFromQueue(first, 4);
    printQueue(first);

    first = deleteFromQueue(first, 3);
    printQueue(first);

    first = addToQueue(first, 5);
    printQueue(first);

    return 1;
}
