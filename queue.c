#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "queue.h"
#include "sem.h"
#include "alloc.h"

/**
 * Семафор для работы с очередью
 */
struct rk_sema *sem;

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
    rk_sema_wait(sem);

    if (maxQueueLength < ++currentQueueLength)
        maxQueueLength = currentQueueLength;

    printf("Start to add n = %d\n", n);
    if (first == NULL) {
        printf("  Add %d: First is null\n", n);
        first = c_calloc(1, sizeof(struct queue));

        first->n = n;
        first->t_time = time(NULL);

    } else {

        struct queue *next = first;
        while (next->q != NULL) {
            next = next->q;
            printf("  Add %d: Next->n = %d\n", n, next->n);
        }

        struct queue *last = c_calloc(1, sizeof(struct queue));
        last->n = n;
        last->t_time = time(NULL);
        next->q = last;
    }
    printf("End of add n = %d\n", n);

    return first;
}

/**
 * Распечатка очереди
 * @param first
 */
char *printQueue(struct queue *first) {
    rk_sema_wait(sem);

    puts("Start printing…");

    char *result, line[20000];
    long int t_time = time(NULL);

    result = c_calloc(sizeof(char), 20000);
    sprintf(line, "%.24s - now\n", ctime(&t_time));
    strcat(result, line);

    struct queue *next = first;
    while (next != NULL) {
        printf("  Print: n = %d\n", next->n);

        sprintf(line, "%.24s - %d\n", ctime(&next->t_time), next->n);
        strcat(result, line);

        next = next->q;
    }

    sprintf(line, "Max concurrency connection: %d\n", maxQueueLength);
    strcat(result, line);

    struct c_countChanges *countChanges = c_result();
    sprintf(line, "Total malloc: %d\n"
                  "Total calloc: %d\n"
                  "Total free: %d\n",
                  countChanges->countMalloc,
                  countChanges->countCalloc,
                  countChanges->countFree);
    strcat(result, line);

    puts("End of print");

    rk_sema_post(sem);

    return result;
}

/**
 * Удаление элемента из очереди
 * @param first
 * @param n
 * @return Ссылка на первывй элемент
 */
struct queue *deleteFromQueue(struct queue *first, int n) {
    rk_sema_wait(sem);

    currentQueueLength--;

    printf("Delete: n = %d\n", n);

    struct queue *next = first;
    struct queue *previous = NULL;

    while (next != NULL) {
        printf("  Delete %d: n = %d\n", n, next->n);

        if (next->n == n) {
            printf("  Delete %d: next->n == n\n", n);
            if (previous == NULL) {
                printf("  Delete %d: Previous is null\n", n);

                first = next->q;
                c_free(next);

                break;
            } else {
                printf("  Delete %d: Previous n = %d\n", n, previous->n);

                previous->q = next->q;
                c_free(next);

                break;
            }
        }

        previous = next;
        next = next->q;
    }

    printf("End of delete %d\n", n);

    return first;
}
