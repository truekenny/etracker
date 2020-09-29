#ifndef SC6_LIST_H
#define SC6_LIST_H

#include "sem.h"

#define STARTING_NEST 0

#define DISABLE_SEMAPHORE 0u
#define ENABLE_SEMAPHORE_LEAF 0b1u
#define ENABLE_SEMAPHORE_GLOBAL 0b10u

// По первым байтам
// #define LITTLE_ENDIAN 0
// #define BIG_ENDIAN 1

struct item {
    unsigned char *hash;

    struct item *previousInLeaf;
    struct item *nextInLeaf;

    struct list *leaf;

    void *data;
};

struct list {
    unsigned char hashLength;
    unsigned char level; // 0 - 2

    // Параметр влияет на то, по каким байтам хэша будет выбираться лист
    // (по первым - LITTLE_ENDIAN или последним - BIG_ENDIAN)
    unsigned short endianness;

    struct list *list;

    unsigned char semaphoreEnabled;
    struct rk_sema *semaphore;

    struct item *firstItem;
};

void waitSemaphoreLeaf(struct list *leaf);

void postSemaphoreLeaf(struct list *leaf);

struct list *getLeaf(struct list *list, unsigned char *hash);

struct list *reInitList(struct list *list, unsigned char level);

struct list *initList(struct list *list, unsigned char level, unsigned char nest, unsigned char hashLength,
                      unsigned char semaphoreEnabled, unsigned short endianness);

void freeList(struct list *list, unsigned char firstRecursion);

unsigned char mapList(struct list *list, void *args,
        unsigned char (*callback)(struct list *list, struct item *item, void *args));

void printList(struct list *list, unsigned char nest);

void deleteItem(struct item *item);

void deleteHash(struct list *list, unsigned char *hash);

struct item *setHash(struct list *list, unsigned char *hash);

struct item *getHash(struct list *list, unsigned char *hash);

struct rk_sema *getLeafSemaphore(struct list *list, unsigned char *hash);

#endif //SC6_LIST_H
