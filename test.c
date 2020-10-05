#include <stdio.h>
#include <stdlib.h>
#include "block.h"
#include "list.h"
#include "alloc.h"
#include "base64.h"

int testBase64();

int testSem();

int testList();

int testBlock();

int main() {
    printf("Starting tests\n");

    printf("testBase64 complete = %d\n", testBase64());
    printf("testSem complete = %d\n", testSem());
    printf("testBlock complete = %d\n", testBlock());
    printf("testList complete = %d\n", testList());
}

int testBase64() {
    struct block *input = NULL;
    struct block *result;

    input = resetBlock(input);
    addStringBlock(input, "1234567890", 10);
    result = base64_encode(input);
    printf("base64_encode(1234567890) > MTIzNDU2Nzg5MA== > %s\n", result->data);
    freeBlock(result);

    input = resetBlock(input);
    addStringBlock(input, "123456789", 9);
    result = base64_encode(input);
    printf("base64_encode(123456789) > MTIzNDU2Nzg5 > %s\n", result->data);
    freeBlock(result);

    input = resetBlock(input);
    addStringBlock(input, "12345678", 8);
    result = base64_encode(input);
    printf("base64_encode(12345678) > MTIzNDU2Nzg= > %s\n", result->data);
    freeBlock(result);

    input = resetBlock(input);
    addStringBlock(input, "1", 1);
    result = base64_encode(input);
    printf("base64_encode(1) > MQ== > %s\n", result->data);
    freeBlock(result);

    input = resetBlock(input);
    addStringBlock(input, "12", 2);
    result = base64_encode(input);
    printf("base64_encode(12) > MTI= > %s\n", result->data);
    freeBlock(result);

    input = resetBlock(input);
    addStringBlock(input, "123", 3);
    result = base64_encode(input);
    printf("base64_encode(123) > MTIz > %s\n", result->data);
    freeBlock(result);

    freeBlock(input);

    return 1;
}

int testSem() {
    struct rk_sema sem;
    rk_sema_init(&sem, 1);
    rk_sema_wait(&sem);
    rk_sema_post(&sem);

    rk_sema_wait(&sem);
    rk_sema_post(&sem);
    rk_sema_destroy(&sem);

    return 1;
}

/**
 * Удаляет первый встретевшийся итем
 * @param list
 * @param item
 * @param args
 * @return
 */
unsigned char listCallback(struct list *list, struct item *item, void *args) {
    deleteHash(list, item->hash);

    return 1;
}

unsigned char listPrintCallback(struct list *list, struct item *item, void *args) {
    printf("f");

    return 0;
}

int testList() {
    struct c_countChanges *c_countChanges = c_result();
    printf("M:%llu C:%llu F:%llu T:%llu\n", c_countChanges->countMalloc, c_countChanges->countCalloc, c_countChanges->countFree,
           c_countChanges->countMalloc + c_countChanges->countCalloc - c_countChanges->countFree);

    struct list *list = NULL;
    list = initList(NULL, 2, STARTING_NEST, 2, ENABLE_SEMAPHORE_LEAF, LITTLE_ENDIAN);

    // Обхожу пустой список
    mapList(list, NULL, &listPrintCallback);

    printList(list, 0);

    const unsigned char addCount = 17;
    unsigned char add[addCount][3] = {
            "14\x00",
            "22\x00",
            "12\x00",
            "25\x00",
            "13\x00",
            "21\x00",
            "11\x00",
            "23\x00",
            "10\x00",
            "24\x00",
            "15\x00",
            "26\x00",
            "16\x00",
            "20\x00",
            "30\x00",
            "30\x00",
            "30\x00",
    };
    for (int index = 0; index < addCount; ++index) {
        printf("Add %*s\n", 2, add[index]);
        setHash(list, add[index]);
        printList(list, 0);
    }

    printf("reInitList\n");
    list = reInitList(list, 1);

    const unsigned char deleteCount = 17;
    unsigned char delete[deleteCount][3] = {
            "14\x00",
            "22\x00",
            "12\x00",
            "25\x00",
            "13\x00",
            "21\x00",
            "11\x00",
            "23\x00",
            "10\x00",
            "24\x00",
            "15\x00",
            "26\x00",
            "16\x00",
            "20\x00",
            "30\x00",
            "30\x00",
            "30\x00",
    };
    for (int index = 0; index < deleteCount; ++index) {
        printf("Delete %*s\n", 2, delete[index]);
        deleteHash(list, delete[index]);
        printList(list, 0);
    }


    const unsigned char addCount2 = 2;
    unsigned char add2[addCount2][3] = {
            "30\x00",
            "31\x00",
    };
    for (int index = 0; index < addCount2; ++index) {
        printf("Add %*s\n", 2, add2[index]);
        setHash(list, add2[index]);
        printList(list, 0);
    }


    printf("Test callback (delete 30)\n");
    mapList(list, NULL, &listCallback);
    printList(list, 0);

    printf("Test callback (delete 31)\n");
    mapList(list, NULL, &listCallback);
    printList(list, 0);

    printf("M:%llu C:%llu F:%llu T:%llu\n", c_countChanges->countMalloc, c_countChanges->countCalloc, c_countChanges->countFree,
           c_countChanges->countMalloc + c_countChanges->countCalloc - c_countChanges->countFree);

    freeList(list, 1);

    printf("M:%llu C:%llu F:%llu T:%llu\n", c_countChanges->countMalloc, c_countChanges->countCalloc, c_countChanges->countFree,
           c_countChanges->countMalloc + c_countChanges->countCalloc - c_countChanges->countFree);

    if (c_countChanges->countMalloc + c_countChanges->countCalloc - c_countChanges->countFree) {
        printf("Not all allocate are free – failure\n");
    }

    return 1;
}

int testBlock() {
    struct block *block = initBlock();

    addStringBlock(block, "12\00045", 5);
    addStringBlock(block, "12345", 5);
    addStringBlock(block, "12345", 5);
    addStringBlock(block, "12345", 5);
    addFormatStringBlock(block, 10, "%d", 67890);
    addFormatStringBlock(block, 10, "%d", 67890);
    addFormatStringBlock(block, 10, "%d", 67890);
    addFormatStringBlock(block, 10, "%d", 67890);

    printf("12_4512345123451234567890678906789067890\n");

    for (int i = 0; i < block->size; i++) {
        if (block->data[i] == 0) {
            printf("_");

            continue;
        }

        printf("%c", block->data[i]);
    }
    printf("\n");

    freeBlock(block);

    return 1;
}
