//
// Created by Kenny on 29.08.20.
//

#ifndef SC6_SEM_H
#define SC6_SEM_H

/**
 * Структура семафора
 */
struct rk_sema;

/**
 * Размер структуры
 * @return
 */
int rk_sema_size();

/**
 * Инициализация семафора
 * @param s
 * @param value
 */
void rk_sema_init(struct rk_sema *s, uint32_t value);

/**
 * Блокирование семафора
 * @param s
 */
void rk_sema_wait(struct rk_sema *s);

/**
 * Освобождение семафора
 * @param s
 */
void rk_sema_post(struct rk_sema *s);

#endif //SC6_SEM_H
