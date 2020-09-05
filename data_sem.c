#include "data_sem.h"

void initSem(struct firstByte *firstByte) {
    int i, j;

    for (i = 0; i < 256; i++) {
        for (j = 0; j < 256; j++) {
            rk_sema_init(&firstByte->secondByte[i].sem[j], 1);
        }
    }
}

void waitSem(struct firstByte *firstByte, struct query *query) {
    struct rk_sema *sem = &firstByte->secondByte[query->info_hash[0]].sem[query->info_hash[1]];
    rk_sema_wait(sem);
}

void postSem(struct firstByte *firstByte, struct query *query) {
    struct rk_sema *sem = &firstByte->secondByte[query->info_hash[0]].sem[query->info_hash[1]];
    rk_sema_post(sem);
}

