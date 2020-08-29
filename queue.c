//
// Created by Kenny on 29.08.20.
//

#include <stdio.h>
#include "queue.h"

/**
 * Очередь
 */
struct queue {
    int n;
    long int t_time;
    struct queue *q;
};

/**
 * Первый элемент очереди
 */
struct queue *first = NULL;

/**
 * Максимальное и текущее кол-во одновременных подключений
 */
int maxQueueLength = 0,
        currentQueueLength = 0;

/**
 * Добавляет элемент в очередь
 * @param n
 */
void addToQueue(int n) {
    rk_sema_wait(sem);

    if(maxQueueLength < ++currentQueueLength)
        maxQueueLength = currentQueueLength;

    printf("Start to add n = %d\n", n);
    if (first == NULL) {
        printf("  Add %d: First is null\n", n);
        first = calloc(1, sizeof(struct queue));

        first->n = n;
        first->t_time = time(NULL);

    } else {

        struct queue *next = first;
        while (next->q != NULL) {
            next = next->q;
            printf("  Add %d: Next->n = %d\n", n, next->n);
        }

        struct queue *last = calloc(1, sizeof(struct queue));
        last->n = n;
        last->t_time = time(NULL);
        next->q = last;
    }
    printf("End of add n = %d\n", n);

    rk_sema_post(sem);
}

/**
 * Распечатка очереди
 */
char *printQueue() {
    rk_sema_wait(sem);

    puts("Start printing…");

    char *result, line[20000];
    long int t_time = time(NULL);

    result = calloc(sizeof(char), 20000);
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

    puts("End of print");

    rk_sema_post(sem);

    return result;
}

/**
 * Удаление элемента из очереди
 * @param n
 */
void deleteFromQueue(int n) {
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
                free(next);

                break;
            } else {
                printf("  Delete %d: Previous n = %d\n", n, previous->n);

                previous->q = next->q;
                free(next);

                break;
            }
        }

        previous = next;
        next = next->q;
    }

    printf("End of delete %d\n", n);

    rk_sema_post(sem);
}
