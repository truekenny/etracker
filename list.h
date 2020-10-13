#ifndef SC6_LIST_H
#define SC6_LIST_H

#include <stdatomic.h>
#include "sem.h"
#include "block.h"

#define LIST_STARTING_NEST 0

#define LIST_SEMAPHORE_DISABLE 0u
#define LIST_SEMAPHORE_ENABLE_LEAF    0b1u
#define LIST_SEMAPHORE_ENABLE_GLOBAL 0b10u

#define LIST_CONTINUE_RETURN 0
#define LIST_BREAK_RETURN    1

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
    struct list *list;                  // Ветки текущего списка веток
    _Atomic (struct item *) firstItem;  // Листья текущего списка листов
    struct rk_sema *semaphore;          // Семафор, обычно для списка листов, реже глобальный

    // Параметр влияет на то, по каким байтам хэша будет выбираться лист
    // (по первым - LITTLE_ENDIAN или последним - BIG_ENDIAN)
    unsigned short endianness;
    unsigned char hashLength;           // Размер хэша item'ов
    unsigned char level;                // Размер хэш таблицы, [0-2]
    unsigned char semaphoreEnabled;     // Использовать семафоры при доступе к списку листов
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

void printList(struct list *list, unsigned char nest, struct block *block, _Bool print);

void deleteItem(struct item *item);

void deleteHash(struct list *list, unsigned char *hash);

struct item *setHash(struct list *list, unsigned char *hash);

struct item *getHash(struct list *list, unsigned char *hash);

struct rk_sema *getLeafSemaphore(struct list *list, unsigned char *hash);

#endif //SC6_LIST_H
