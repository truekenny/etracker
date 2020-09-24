#ifndef SC6_ALLOC_H
#define SC6_ALLOC_H

#include <stdatomic.h>

struct c_countChanges {
    atomic_ullong countMalloc;
    atomic_ullong countCalloc;
    atomic_ullong countFree;
};

void c_initSem();

void *c_malloc(int size);

void *c_calloc(int count, int size);

void c_free(void *pointer);

void *c_realloc(void *pointer, size_t size);

struct c_countChanges *c_result();

#endif //SC6_ALLOC_H
