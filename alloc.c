#include <stdio.h>
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

    void *result = malloc(size);
    if (result == NULL) {
        printf("Malloc failure, size = %d\n", size);
    }

    return result;
}

void *c_calloc(int count, int size) {
    DEBUG_STRICT_MEM && rk_sema_wait(&semAlloc);
    countChanges.countCalloc++;
    DEBUG_STRICT_MEM && rk_sema_post(&semAlloc);

    void *result = calloc(count, size);
    if (result == NULL) {
        printf("Calloc failure, count = %d, size = %d\n", count, size);
    }

    return result;
}

void c_free(void *pointer) {
    DEBUG_STRICT_MEM && rk_sema_wait(&semAlloc);
    countChanges.countFree++;
    DEBUG_STRICT_MEM && rk_sema_post(&semAlloc);

    free(pointer);
}

void *c_realloc(void *pointer, size_t size) {
    void *result = realloc(pointer, size);

    if(result == NULL) {
        printf("Realloc failure, needSize = %zu\n", size);
    }

    return result;
}


struct c_countChanges *c_result() {
    return &countChanges;
}
