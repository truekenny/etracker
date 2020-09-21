#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include "list.h"
#include "alloc.h"

#define LIMIT_LEVEL 2
#define DEC_BYTE 256

unsigned char reInitListCallback(struct list *list, struct item *item, void *args) {
    if (list == NULL) {
        // unused
    }

    struct list *resultList = args;

    struct item *newItem = setHash(resultList, item->hash);

    // Сохраняю данные в новом item
    newItem->data = item->data;
    // В старом теряю указатель, чтобы не освободить в функции удаления item
    item->data = NULL;
    deleteItem(item);

    return 0;
}

struct list *reInitList(struct list *list, unsigned char level) {
    struct list *resultList =
            initList(NULL, level, STARTING_NEST, list->hashLength, list->semaphoreEnabled, list->endianness);

    mapList(list, resultList, &reInitListCallback);
    freeList(list, 1);

    return resultList;
}

struct list *initList(struct list *list, unsigned char level, unsigned char nest, unsigned char hashLength,
                      unsigned char semaphoreEnabled, unsigned short endianness) {

    if (level > LIMIT_LEVEL) {
        printf("Level limit = %d, current = %d\n", LIMIT_LEVEL, level);

        exit(55);
    }

    if (level > hashLength) {
        printf("level = %d > hashLength = %d – failure\n", LIMIT_LEVEL, hashLength);

        exit(56);
    }

    if (list == NULL)
        list = c_calloc(1, sizeof(struct list));

    list->level = level;
    list->hashLength = hashLength;
    list->semaphoreEnabled = semaphoreEnabled;
    list->endianness = endianness;

    if (level > 0) {
        list->list = c_calloc(DEC_BYTE, sizeof(struct list));

        for (int index = 0; index < DEC_BYTE; ++index) {
            initList(&list->list[index], level - 1, nest + 1, hashLength,
                     semaphoreEnabled, endianness);
        }
    }

    if (level == 0 && list->semaphoreEnabled) {
        list->semaphore = c_calloc(1, sizeof(struct rk_sema));
        rk_sema_init(list->semaphore, 1);
    }

    return list;
}

void freeList(struct list *list, unsigned char firstRecursion) {
    if (list->firstItem != NULL) {
        printf("List not empty\n");

        exit(77);
    }

    unsigned char level = list->level;

    if (level > 0) {
        for (int index = 0; index < DEC_BYTE; ++index) {
            freeList(&list->list[index], 0);
        }

        c_free(list->list);
    }

    if (level == 0 && list->semaphoreEnabled) {
        rk_sema_destroy(list->semaphore);
        c_free(list->semaphore);
    }

    if (firstRecursion)
        c_free(list);
}

unsigned char mapList(struct list *list, void *args,
                      unsigned char (*callback)(struct list *list, struct item *item, void *args)) {
    if (list == NULL)
        return 0;

    if (list->level > 0) {
        for (int index = 0; index < DEC_BYTE; ++index) {
            unsigned char test = mapList(&list->list[index], args, callback);

            if (test)
                return 1;
        }
    } else {
        if (list->firstItem) {
            waitSemaphoreLeaf(list);

            struct item *currentItem = list->firstItem;

            while (currentItem != NULL) {
                unsigned char test = (*callback)(list, currentItem, args);

                if (test) {
                    postSemaphoreLeaf(list);

                    return 1;
                }

                currentItem = currentItem->nextInLeaf;
            }

            postSemaphoreLeaf(list);
        }
    }

    return 0;
}

void printList(struct list *list, unsigned char nest) {
    if (nest == 0) {
        printf("List:");
    }

    if (list->level > 0) {
        for (int index = 0; index < DEC_BYTE; ++index) {
            printList(&list->list[index], nest + 1);
        }
    } else {
        // Печать листа, у которого есть элемент
        if (list->firstItem) {
            waitSemaphoreLeaf(list);

            struct item *currentItem = list->firstItem;

            while (currentItem != NULL) {
                printf("%*s ", list->hashLength, currentItem->hash);

                currentItem = currentItem->nextInLeaf;
            }

            printf(", ");
            postSemaphoreLeaf(list);
        }
    }

    if (nest == 0) {
        printf("\n");
    }
}

void deleteItem(struct item *item) {
    if (item->previousInLeaf == NULL) {
        item->leaf->firstItem = item->nextInLeaf;
        if (item->nextInLeaf != NULL) {
            item->nextInLeaf->previousInLeaf = NULL;
        }
    } else {
        item->previousInLeaf->nextInLeaf = item->nextInLeaf;
        if (item->nextInLeaf != NULL) {
            item->nextInLeaf->previousInLeaf = item->previousInLeaf;
        }
    }

    if (item->data != NULL) {
        c_free(item->data);
    }
    c_free(item->hash);
    c_free(item);
}

void deleteHash(struct list *list, unsigned char *hash) {
    struct item *item = getHash(list, hash);

    if (item != NULL)
        deleteItem(item);
}

/*
 * todo: добавить setHashLeaf, поскольку leaf получаю для блокирования семафора;
 *  полагаю, что эта оптимизация не принесет большой оптимизации, так как getLeaf почти линейна
 */
struct item *setHash(struct list *list, unsigned char *hash) {
    struct list *leaf = getLeaf(list, hash);

    struct item *item = NULL;

    struct item *previousItem = NULL;
    struct item *currentItem = leaf->firstItem;
    while (currentItem != NULL) {
        int hashCompare = memcmp(currentItem->hash, hash, leaf->hashLength);
        if (hashCompare == 0) {
            item = currentItem;

            break;
        } else if (hashCompare > 0) {

            break;
        }

        previousItem = currentItem;
        currentItem = currentItem->nextInLeaf;
    }

    if (item == NULL) {
        item = c_calloc(1, sizeof(struct item));

        item->hash = c_malloc(leaf->hashLength);
        memcpy(item->hash, hash, leaf->hashLength);

        item->leaf = leaf;

        // Добавить в лист
        item->previousInLeaf = previousItem;
        item->nextInLeaf = currentItem;

        // Обновить соседей в ветке
        if (previousItem != NULL)
            previousItem->nextInLeaf = item;

        if (currentItem != NULL)
            currentItem->previousInLeaf = item;

        if (previousItem == NULL)
            leaf->firstItem = item;
    }

    return item;
}

struct item *getHash(struct list *list, unsigned char *hash) {
    struct list *leaf = getLeaf(list, hash);

    struct item *currentItem = leaf->firstItem;
    while (currentItem != NULL) {
        int hashCompare = memcmp(currentItem->hash, hash, leaf->hashLength);
        if (hashCompare == 0) {

            return currentItem;
        } else if (hashCompare > 0) {

            break;
        }

        currentItem = currentItem->nextInLeaf;
    }

    return NULL;
}

struct rk_sema *getLeafSemaphore(struct list *list, unsigned char *hash) {
    struct list *leaf = getLeaf(list, hash);

    return leaf->semaphore;
}

struct list *getLeaf(struct list *list, unsigned char *hash) {
    struct list *currentList = list;

    unsigned char nest = 0;

    while (currentList->level > 0) {
        if (list->endianness == BIG_ENDIAN)
            currentList = &currentList->list[hash[list->hashLength - 1 - nest]];
        else
            currentList = &currentList->list[hash[nest]];
        nest++;
    }

    return currentList;
}

void waitSemaphoreLeaf(struct list *leaf) {
    if (leaf->semaphoreEnabled) {
        rk_sema_wait(leaf->semaphore);
    }
}

void postSemaphoreLeaf(struct list *leaf) {
    if (leaf->semaphoreEnabled) {
        rk_sema_post(leaf->semaphore);
    }
}
