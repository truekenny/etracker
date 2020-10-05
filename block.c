#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include "block.h"
#include "alloc.h"
#include "math.h"

#define DEBUG 0
// Начальный размер памяти
#define START_ALLOCATE_SIZE 2000
// Минимальный размер оставшейся свободной памяти (если выделяеься точный предел)
#define MIN_FREE_ALLOCATE_SIZE 2000
// Каждый раз расширяю память минимум в два раза
#define MIN_RE_ALLOC_MULTIPLY 2

void reAllocBlock(struct block *block, unsigned int requiredSpace);

void checkSizeBlock(struct block *block);

struct block *initBlock() {
    struct block *block = c_calloc(1, sizeof(struct block));
    block->data = c_malloc(START_ALLOCATE_SIZE * sizeof(char));
    block->allocated = START_ALLOCATE_SIZE;
    block->data[0] = 0;

    return block;
}

void freeBlock(struct block *block) {
    if (block == NULL) {
        printf("Cant freeBlock, block == NULL\n");

        return;
    }

    c_free(block->data);
    c_free(block);
}

struct block *resetBlock(struct block *block) {
    freeBlock(block);

    return initBlock();
}

void addStringBlock(struct block *block, void *string, unsigned int requiredSpace) {
    reAllocBlock(block, requiredSpace);

    memcpy(&block->data[block->size], string, requiredSpace);
    block->size += requiredSpace;

    // Для printf
    block->data[block->size] = 0;

    checkSizeBlock(block);
}

void addFormatStringBlock(struct block *block, unsigned int requiredSpace, char *format, ...) {
    reAllocBlock(block, requiredSpace);

    va_list argList;
    va_start(argList, format);
    block->size += vsprintf(&block->data[block->size], format, argList);
    va_end(argList);

    // Для printf
    block->data[block->size] = 0;

    checkSizeBlock(block);
}

void addFileBlock(struct block *block, unsigned int requiredSpace, char *filename) {
    reAllocBlock(block, requiredSpace);

    FILE *file = fopen(filename, "rb");

    block->size += fread(&block->data[block->size], 1, requiredSpace, file);
    fclose(file);

    // Для printf
    block->data[block->size] = 0;

    checkSizeBlock(block);
}

void reAllocBlock(struct block *block, unsigned int requiredSpace) {
    /*
     * Проверка свободного места
     * Знак сравнения без =, чтобы осталось место как минимум на 1 символ после и он будет \0
     */
    if (block->allocated > block->size + requiredSpace) {
        return;
    }

    // Новый размер выделенной памяти
    unsigned int newAlloc = max(
            block->allocated + requiredSpace + MIN_FREE_ALLOCATE_SIZE,
            block->allocated * MIN_RE_ALLOC_MULTIPLY);

    if (newAlloc) {
        DEBUG && printf("ReAlloc block: %d -> %d\n", block->allocated, newAlloc);

        block->data = c_realloc(block->data, newAlloc);
        block->allocated = newAlloc;
    }
}

void checkSizeBlock(struct block *block) {
    if (block->size < block->allocated)
        return;

    printf("block->size >= block->allocated\n");
}
