#ifndef SC6_ALLOC_H
#define SC6_ALLOC_H

#include <stdatomic.h>

struct c_countChanges {
    atomic_int countMalloc;
    atomic_int countCalloc;
    atomic_int countFree;
};

void c_initSem();

void *c_malloc(int size);

void *c_calloc(int count, int size);

void c_free(void *pointer);

struct c_countChanges *c_result();

#endif //SC6_ALLOC_H
