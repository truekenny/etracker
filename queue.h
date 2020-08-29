//
// Created by Kenny on 29.08.20.
//

#ifndef SC6_QUEUE_H
#define SC6_QUEUE_H

/**
 * Семафор для работы с очередью
 */
struct rk_sema *sem;

/**
 * Добавляет элемент в очередь
 * @param n
 */
void addToQueue(int n);

/**
 * Распечатка очереди
 */
char *printQueue();

/**
 * Удаление элемента из очереди
 * @param n
 */
void deleteFromQueue(int n);

#endif //SC6_QUEUE_H
