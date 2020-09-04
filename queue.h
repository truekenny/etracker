#ifndef SC6_QUEUE_H
#define SC6_QUEUE_H

/**
 * Структура элемента очереди
 */
struct queue;

/**
 * Добавляет элемент в очередь
 * @param first
 * @param n
 * @return Ссылка на первый элемент
 */
struct queue * addToQueue(struct queue *first, int n);

/**
 * Распечатка очереди
 * @param first
 */
struct block *printQueue(struct queue *first);

/**
 * Удаление элемента из очереди
 * @param first
 * @param n
 * @return Ссылка на первый элемент
 */
struct queue * deleteFromQueue(struct queue *first, int n);

#endif //SC6_QUEUE_H
