#include "data_sem.h"

void initSem(struct firstByteData *firstByte) {
    int i, j;

    for (i = 0; i < 256; i++) {
        for (j = 0; j < 256; j++) {
            rk_sema_init(&firstByte->secondByteData[i].semaphore[j], 1);
        }
    }
}

void waitSem(struct firstByteData *firstByte, struct query *query) {
    struct rk_sema *sem = &firstByte->secondByteData[query->info_hash[0]].semaphore[query->info_hash[1]];
    rk_sema_wait(sem);
}

void postSem(struct firstByteData *firstByte, struct query *query) {
    struct rk_sema *sem = &firstByte->secondByteData[query->info_hash[0]].semaphore[query->info_hash[1]];
    rk_sema_post(sem);
}

