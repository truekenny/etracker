#ifndef SC6_ALLOC_H
#define SC6_ALLOC_H

//struct c_countChanges;
struct c_countChanges {
    int countMalloc;
    int countCalloc;
    int countFree;
};

void *c_malloc(int size);

void *c_calloc(int count, int size);

void c_free(void *pointer);

struct c_countChanges *c_result();

#endif //SC6_ALLOC_H
