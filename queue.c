#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "queue.h"
#include "alloc.h"

#define DEBUG 0
#define MAX_LINE_LENGTH 1000
#define MAX_RESULT_LENGTH 19000
#define MAX_END_RESULT_LENGTH 1000

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

    return first;
}

/**
 * Распечатка очереди
 * @param first
 */
char *printQueue(struct queue *first) {
    if (DEBUG)
        printf("Start printing…\n");

    char *result, line[MAX_LINE_LENGTH];
    long int t_time = time(NULL);

    result = c_calloc(sizeof(char), MAX_RESULT_LENGTH);
    sprintf(line, "%.24s - now\n", ctime(&t_time));
    strcat(result, line);

    struct queue *next = first;
    while (next != NULL) {
        DEBUG && printf("  Print: n = %d\n", next->n);

        sprintf(line, "%.24s - %d\n", ctime(&next->t_time), next->n);
        if (strlen(result) + strlen(line) > MAX_RESULT_LENGTH - MAX_END_RESULT_LENGTH) {
            DEBUG && printf("Error: Result too long: n = %d\n", next->n);

            exit(1);
            // return result;
        }
        strcat(result, line);

        next = next->q;
    }

    // End of result
    struct c_countChanges *countChanges = c_result();
    sprintf(line, "Max concurrency connection: %d\n"
                  "Total malloc: %d\n"
                  "Total calloc: %d\n"
                  "Total *alloc: %d\n"
                  "Total free: %d\n"
                  "Total *alloc - free: %d\n",
            maxQueueLength,
            countChanges->countMalloc,
            countChanges->countCalloc,
            countChanges->countMalloc + countChanges->countCalloc,
            countChanges->countFree,
            countChanges->countMalloc + countChanges->countCalloc - countChanges->countFree);
    strcat(result, line);

    DEBUG && puts("End of print");

    return result;
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

    struct queue *next = first;
    struct queue *previous = NULL;

    _Bool hasDelete = 0;

    while (next != NULL) {
        DEBUG && printf("  Delete %d: n = %d\n", n, next->n);

        if (next->n == n) {
            hasDelete = 1;

            DEBUG && printf("  Delete %d: next->n == n\n", n);
            if (previous == NULL) {
                DEBUG && printf("  Delete %d: Previous is null\n", n);

                first = next->q;
                c_free(next);

                break;
            } else {
                DEBUG && printf("  Delete %d: Previous n = %d\n", n, previous->n);

                previous->q = next->q;
                c_free(next);

                break;
            }
        }

        previous = next;
        next = next->q;
    }

    DEBUG && printf("End of delete %d\n", n);

    if (!hasDelete) {
        printf("ERROR: Can not delete n = %d\n", n);

        exit(1);
    }

    return first;
}
