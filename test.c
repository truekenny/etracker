#include <stdio.h>
#include "queue.h"

struct queue *first = NULL;

void testQueue();

int main() {
    testQueue();
    printf("Test complete\n");
}

/**
 * Тестирует очередь
 */
void testQueue() {
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
}
