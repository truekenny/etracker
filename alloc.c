#include <stdlib.h>
#include "alloc.h"

struct c_countChanges countChanges;

void *c_malloc(int size) {
    countChanges.countMalloc++;

    return malloc(size);
}

void *c_calloc(int count, int size) {
    countChanges.countCalloc++;

    return calloc(count, size);
}

void c_free(void *pointer) {
    countChanges.countFree++;

    free(pointer);
}

struct c_countChanges *c_result() {
    return &countChanges;
}
