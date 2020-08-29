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
 * Структура элемента очереди
 */
struct queue;

/**
 * Добавляет элемент в очередь
 * @param first
 * @param n
 */
void addToQueue(struct queue *first, int n);

/**
 * Распечатка очереди
 * @param first
 */
char *printQueue(struct queue *first);

/**
 * Удаление элемента из очереди
 * @param first
 * @param n
 */
void deleteFromQueue(struct queue *first, int n);

#endif //SC6_QUEUE_H
