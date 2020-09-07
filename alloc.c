#include <stdlib.h>
#include "alloc.h"
#include "sem.h"

#define DEBUG_STRICT_MEM 0

struct rk_sema semAlloc = {0};

struct c_countChanges countChanges;

void c_initSem() {
    DEBUG_STRICT_MEM && rk_sema_init(&semAlloc, 1);
}

void *c_malloc(int size) {
    DEBUG_STRICT_MEM && rk_sema_wait(&semAlloc);
    countChanges.countMalloc++;
    DEBUG_STRICT_MEM && rk_sema_post(&semAlloc);

    return malloc(size);
}

void *c_calloc(int count, int size) {
    DEBUG_STRICT_MEM && rk_sema_wait(&semAlloc);
    countChanges.countCalloc++;
    DEBUG_STRICT_MEM && rk_sema_post(&semAlloc);

    return calloc(count, size);
}

void c_free(void *pointer) {
    DEBUG_STRICT_MEM && rk_sema_wait(&semAlloc);
    countChanges.countFree++;
    DEBUG_STRICT_MEM && rk_sema_post(&semAlloc);

    free(pointer);
}

struct c_countChanges *c_result() {
    return &countChanges;
}
