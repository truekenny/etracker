#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include "block.h"
#include "alloc.h"

#define DEBUG 0
// Начальный размер памяти
#define START_ALLOCATE_SIZE 1000
// Каждый раз расширяю память минимум в два раза
#define MIN_RE_ALLOC_MULTIPLY 2
#define max(a, b) ({ __typeof__ (a) _a = (a); __typeof__ (b) _b = (b); _a > _b ? _a : _b; })

void reAllocBlock(struct block *block, unsigned int requiredSpace);

struct block *initBlock() {
    struct block *block = c_calloc(1, sizeof(struct block));
    block->data = c_calloc(START_ALLOCATE_SIZE, sizeof(char));
    block->allocated = START_ALLOCATE_SIZE;

    return block;
}

void freeBlock(struct block *block) {
    c_free(block->data);
    c_free(block);
}

void addStringBlock(struct block *block, char *string, unsigned int requiredSpace) {
    reAllocBlock(block, requiredSpace);

    memcpy(&block->data[block->size], string, requiredSpace);
    block->size += requiredSpace;
}

void addFormatStringBlock(struct block *block, unsigned int requiredSpace, char *format, ...) {
    reAllocBlock(block, requiredSpace);

    va_list argList;
    va_start(argList, format);
    block->size += vsprintf(&block->data[block->size], format, argList);
    va_end(argList);
}

void reAllocBlock(struct block *block, unsigned int requiredSpace) {
    // Место есть
    if (block->size + requiredSpace < block->allocated) {
        return;
    }

    // Новый размер выделенной памяти
    unsigned int newAlloc = max(block->allocated + requiredSpace, MIN_RE_ALLOC_MULTIPLY * block->allocated);

    if (newAlloc) {
        DEBUG && printf("ReAlloc block: %d -> %d\n", block->allocated, newAlloc);

        block->data = realloc(block->data, newAlloc);
        block->allocated = newAlloc;
    }
}
