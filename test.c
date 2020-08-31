#include <stdio.h>
#include "queue.h"

struct queue *first = NULL;

int testQueue();

int main() {
    printf("testQueue complete = %d\n", testQueue());
}

/**
 * Тестирует очередь
 */
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
