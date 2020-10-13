#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include "list.h"
#include "alloc.h"
#include "exit_code.h"

#define LIST_LIMIT_LEVEL 2
#define LIST_BYTE_SIZE 256

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

    return LIST_CONTINUE_RETURN;
}

struct list *reInitList(struct list *list, unsigned char level) {
    struct list *resultList =
            initList(NULL, level, LIST_STARTING_NEST, list->hashLength, list->semaphoreEnabled, list->endianness);

    mapList(list, resultList, &reInitListCallback);
    freeList(list, 1);

    return resultList;
}

struct list *initList(struct list *list, unsigned char level, unsigned char nest, unsigned char hashLength,
                      unsigned char semaphoreEnabled, unsigned short endianness) {

    if (level > LIST_LIMIT_LEVEL) {
        printf("Level limit = %d, current = %d\n", LIST_LIMIT_LEVEL, level);
        exitPrint(EXIT_CODE_LIST_WRONG_LIMIT, __FILE__, EXIT_CODE_PRINT_ERROR_NO);
    }

    if (level > hashLength) {
        printf("level = %d > hashLength = %d – failure\n", LIST_LIMIT_LEVEL, hashLength);

        exitPrint(EXIT_CODE_LIST_WRONG_HASH_LENGTH, __FILE__, EXIT_CODE_PRINT_ERROR_NO);
    }

    if (list == NULL)
        list = c_calloc(1, sizeof(struct list));

    list->level = level;
    list->hashLength = hashLength;
    list->semaphoreEnabled = semaphoreEnabled;
    list->endianness = endianness;

    if (level > 0) {
        list->list = c_calloc(LIST_BYTE_SIZE, sizeof(struct list));

        for (int index = 0; index < LIST_BYTE_SIZE; ++index) {
            initList(&list->list[index], level - 1, nest + 1, hashLength,
                     semaphoreEnabled, endianness);
        }
    }

    if (
            (level == 0 && list->semaphoreEnabled & LIST_SEMAPHORE_ENABLE_LEAF)
            || (nest == LIST_STARTING_NEST && list->semaphoreEnabled & LIST_SEMAPHORE_ENABLE_GLOBAL)
            ) {
        list->semaphore = c_calloc(1, sizeof(struct rk_sema));
        rk_sema_init(list->semaphore, 1);
    }

    return list;
}

void freeList(struct list *list, unsigned char firstRecursion) {
    if (list->firstItem != NULL) {
        printf("List not empty\n");

        exitPrint(EXIT_CODE_LIST_NOT_EMPTY, __FILE__, EXIT_CODE_PRINT_ERROR_NO);
    }

    unsigned char level = list->level;

    if (level > 0) {
        for (int index = 0; index < LIST_BYTE_SIZE; ++index) {
            freeList(&list->list[index], 0);
        }

        c_free(list->list);
    }

    if (
            (level == 0 && list->semaphoreEnabled & LIST_SEMAPHORE_ENABLE_LEAF)
            || (firstRecursion && list->semaphoreEnabled & LIST_SEMAPHORE_ENABLE_GLOBAL)
            ) {
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
        for (int index = 0; index < LIST_BYTE_SIZE; ++index) {
            unsigned char test = mapList(&list->list[index], args, callback);

            if (test == LIST_BREAK_RETURN)
                return LIST_BREAK_RETURN;
        }
    } else {
        if (list->firstItem) {
            waitSemaphoreLeaf(list);

            struct item *currentItem = list->firstItem;

            while (currentItem != NULL) {
                struct item *nextItem = currentItem->nextInLeaf;
                unsigned char test = (*callback)(list, currentItem, args);

                if (test == LIST_BREAK_RETURN) {
                    postSemaphoreLeaf(list);

                    return LIST_BREAK_RETURN;
                }

                currentItem = nextItem;
            }

            postSemaphoreLeaf(list);
        }
    }

    return 0;
}

/**
 * Только для проверки
 * @param list
 * @param nest
 */
void printList(struct list *list, unsigned char nest, struct block *block, _Bool print) {
    if (nest == 0) {
        print && printf("List:");
        addFormatStringBlock(block, 100, "List:");
    }

    if (list->level > 0) {
        for (int index = 0; index < LIST_BYTE_SIZE; ++index) {
            printList(&list->list[index], nest + 1, block, print);
        }
    } else {
        // Печать листа, у которого есть элемент
        if (list->firstItem) {
            waitSemaphoreLeaf(list);

            struct item *currentItem = list->firstItem;

            while (currentItem != NULL) {
                // Здесь не поставил точку и наблюдал в консоле атрефакты
                print && printf("%.*s ", list->hashLength, currentItem->hash);
                addFormatStringBlock(block, 100, "%.*s ", list->hashLength, currentItem->hash);

                currentItem = currentItem->nextInLeaf;
            }

            print && printf(", ");
            addFormatStringBlock(block, 100, ", ");
            postSemaphoreLeaf(list);
        }
    }

    if (nest == 0) {
        print && printf("\n");
        addFormatStringBlock(block, 100, "\n");
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
