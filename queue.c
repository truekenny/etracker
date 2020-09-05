#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "queue.h"
#include "alloc.h"

#define DEBUG 0

/**
 * Очередь
 */
struct queue {
    int n;
    long int t_time;
    struct queue *q;
};

/**
 * Максимальное и текущее кол-во одновременных подключений
 */
int maxQueueLength = 0,
        currentQueueLength = 0;

/**
 * @param first
 * @return Размер очереди
 */
int getSize(struct queue *first);

/**
 * Добавляет элемент в очередь
 * @param first
 * @param n
 * @return Ссылка на первый элемент
 */
struct queue *addToQueue(struct queue *first, int n) {
    if (maxQueueLength < ++currentQueueLength)
        maxQueueLength = currentQueueLength;

    DEBUG && printf("Start to add n = %d\n", n);
    struct queue *newFirst = c_calloc(1, sizeof(struct queue));
    newFirst->n = n;
    newFirst->t_time = time(NULL);
    newFirst->q = first;

    first = newFirst;
    DEBUG && printf("End of add n = %d\n", n);

    DEBUG && printf("Size after add: %d\n", getSize(first));

    return first;
}

/**
 * Распечатка очереди
 * @param first
 */
void printQueue(struct block *block, struct queue *first) {
    if (DEBUG) {
        printf("Start printing…\n");
    }

    long int t_time = time(NULL);

    addFormatStringBlock(block, 1000, "%.24s - now\n", ctime(&t_time));

    struct queue *next = first;
    while (next != NULL) {
        DEBUG && printf("  Print: n = %d\n", next->n);

        addFormatStringBlock(block, 1000, "%.24s - %d\n", ctime(&next->t_time), next->n);

        next = next->q;
    }

    // End of result
    struct c_countChanges *countChanges = c_result();
    addFormatStringBlock(block, 2000,
                         "Max concurrency connection = %d\n"
                         "Total malloc = %d\n"
                         "Total calloc = %d\n"
                         "Total *alloc = %d\n"
                         "Total free = %d\n"
                         "Total *alloc - free = %d\n\n",
                         maxQueueLength,
                         countChanges->countMalloc,
                         countChanges->countCalloc,
                         countChanges->countMalloc + countChanges->countCalloc,
                         countChanges->countFree,
                         countChanges->countMalloc + countChanges->countCalloc - countChanges->countFree);

    DEBUG && puts("End of print");
}

/**
 * Удаление элемента из очереди
 * @param first
 * @param n
 * @return Ссылка на первывй элемент
 */
struct queue *deleteFromQueue(struct queue *first, int n) {
    currentQueueLength--;

    DEBUG && printf("Delete: n = %d\n", n);

    _Bool hasDelete = 0;

    if (first == NULL) {
        DEBUG && printf("  Delete %d: first = NULL\n", n);
    } else if (first->n == n) {
        DEBUG && printf("  Delete %d: first deleted\n", n);

        hasDelete = 1;
        struct queue *forFree = first;
        first = first->q;

        c_free(forFree);
    } else {
        struct queue *next = first->q;
        struct queue *previous = first;

        while (next != NULL) {
            DEBUG && printf("  Delete %d: n = %d\n", n, next->n);

            if (next->n == n) {
                DEBUG && printf("  Delete %d: next->n == n\n", n);
                DEBUG && printf("  Delete %d: Previous n = %d\n", n, previous->n);

                hasDelete = 1;

                previous->q = next->q;
                c_free(next);

                break;
            }

            previous = next;
            next = next->q;
        }
    }

    DEBUG && printf("End of delete %d\n", n);

    if (!hasDelete) {
        printf("ERROR: Can not delete n = %d\n", n);

        exit(20);
    }

    DEBUG && printf("Size after delete: %d\n", getSize(first));

    return first;
}

/**
 * @param first
 * @return Размер очереди
 */
int getSize(struct queue *first) {
    int size = 0;

    struct queue *next = first;
    while (next != NULL) {
        next = next->q;
        size++;
    }

    return size;
}
