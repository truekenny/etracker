//
// Created by Kenny on 29.08.20.
//

#ifndef SC6_SEM_H
#define SC6_SEM_H

struct rk_sema;

int rk_sema_size();

void rk_sema_init(struct rk_sema *s, uint32_t value);

void rk_sema_wait(struct rk_sema *s);

void rk_sema_post(struct rk_sema *s);

#endif //SC6_SEM_H
