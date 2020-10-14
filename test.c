#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <assert.h>
#include "block.h"
#include "list.h"
#include "alloc.h"
#include "base64.h"
#include "string.h"
#include "exit_code.h"
#include "data_structure.h"
#include "sha1.h"
#include "string.h"
#include "geoip.h"
#include "exit_code.h"

int testGeoip();

int testStructure();

int testSha1();

int testBase64();

int testSem();

int testList();

int testBlock();

int main() {
    printf("Starting tests\n");

    printf("testStructure complete = %d\n", testStructure());
    printf("testGeoip complete = %d\n", testGeoip());
    printf("testSha1 complete = %d\n", testSha1());
    printf("testBase64 complete = %d\n", testBase64());
    printf("testSem complete = %d\n", testSem());
    printf("testBlock complete = %d\n", testBlock());
    printf("testList complete = %d\n", testList());
}

int testStructure() {
    printf("list=%lu item=%lu torrent=%lu peer=%lu\n",
           sizeof(struct list), sizeof(struct item), sizeof(struct torrentDataL), sizeof(struct peerDataL));

    printf("before changes: 40 40 24 24\n");
    printf("after  changes: 32 40 24 16\n");

    return 1;
}

int testGeoip() {
    struct geoip *geoip = initGeoip();
    loadGeoip(geoip);

    unsigned int ip = 1570630176;
    printf("ip:%u\n", ip);
    struct geoip *middle = findGeoip(geoip, ip);
    printf(" s:%u e:%u %f %f\n", middle->startIp, middle->endIp, middle->lat, middle->lon);
    assert(middle->startIp <= ip);
    assert(ip <= middle->endIp);

    ip = 4294967295;
    printf("ip:%u\n", ip);
    middle = findGeoip(geoip, ip);
    printf(" s:%u e:%u %f %f\n", middle->startIp, middle->endIp, middle->lat, middle->lon);
    assert(middle->startIp <= ip);
    assert(ip <= middle->endIp);

    ip = 0;
    printf("ip:%u\n", ip);
    middle = findGeoip(geoip, ip);
    printf(" s:%u e:%u %f %f\n", middle->startIp, middle->endIp, middle->lat, middle->lon);
    assert(middle->startIp <= ip);
    assert(ip <= middle->endIp);

    printf("start loop\n");
    for (int i = 0; i < 256000; ++i) {
        ip = (unsigned int) rand();
        middle = findGeoip(geoip, ip);
        assert(middle->startIp <= ip);
        assert(ip <= middle->endIp);
    }
    printf("end loop\n");

    freeGeoip(geoip);

    return 1;
}

int testSha1() {
    struct block *block = NULL;

    block = resetBlock(block);
    addStringBlock(block, "1", 1);
    sha1(block);
    assert(memcmp(block->data, "\x35\x6a\x19\x2b\x79\x13\xb0\x4c\x54\x57\x4d\x18\xc2\x8d\x46\xe6\x39\x54\x28\xab",
                  block->size) == 0);

    block = resetBlock(block);
    addStringBlock(block, "abc", 3);
    sha1(block);
    assert(memcmp(block->data, "\xa9\x99\x3e\x36\x47\x06\x81\x6a\xba\x3e\x25\x71\x78\x50\xc2\x6c\x9c\xd0\xd8\x9d",
                  block->size) == 0);

    block = resetBlock(block);
    addStringBlock(block, "123456789012345678901234567890123456789012345678901234567890", 60);
    sha1(block);
    assert(memcmp(block->data, "\x24\x5b\xe3\x00\x91\xfd\x39\x2f\xe1\x91\xf4\xbf\xce\xc2\x2d\xcb\x30\xa0\x3a\xe6",
                  block->size) == 0);

    block = resetBlock(block);
    addStringBlock(block, "1234567890123456789012345678901234567890123456789012345678901234567890", 70);
    sha1(block);
    assert(memcmp(block->data, "\x61\x0e\x97\x3d\x05\x14\x5f\xd4\xc3\xe6\xc9\x7f\xf3\x49\x90\x7f\xdc\x8e\xc4\xb7",
                  block->size) == 0);

    block = resetBlock(block);
    addStringBlock(block,
                   "12345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890",
                   140);
    sha1(block);
    assert(memcmp(block->data, "\xf0\x02\x6e\x50\xbe\xa5\xe9\x0d\xd1\x13\xd9\x33\x31\x2a\x8a\x6c\x7f\xb0\x6f\x7b",
                  block->size) == 0);

    block = resetBlock(block);
    // ZERO BYTES TEST
    sha1(block);
    assert(memcmp(block->data, "\xda\x39\xa3\xee\x5e\x6b\x4b\x0d\x32\x55\xbf\xef\x95\x60\x18\x90\xaf\xd8\x07\x09",
                  block->size) == 0);

    // 256 bytes 0…\xFF
    block = resetBlock(block);
    unsigned char buff[256];
    for (int i = 0; i < 256; ++i) {
        buff[i] = i;
    }
    addStringBlock(block,
                   buff,
                   256);
    sha1(block);
    assert(memcmp(block->data, "\x49\x16\xd6\xbd\xb7\xf7\x8e\x68\x03\x69\x8c\xab\x32\xd1\x58\x6e\xa4\x57\xdf\xc8",
                  block->size) == 0);

    // 256 bytes 0…0
    block = resetBlock(block);
    for (int i = 0; i < 256; ++i) {
        buff[i] = 0;
    }
    addStringBlock(block,
                   buff,
                   256);
    sha1(block);
    assert(memcmp(block->data, "\xb3\x76\x88\x5a\xc8\x45\x2b\x6c\xbf\x9c\xed\x81\xb1\x08\x0b\xfd\x57\x0d\x9b\x91",
                  block->size) == 0);

    freeBlock(block);

    return 1;
}

int testBase64() {
    struct block *input = NULL;
    struct block *result;

    input = resetBlock(input);
    addStringBlock(input, "1234567890", 10);
    result = base64_encode(input);
    assert(memcmp(result->data, "MTIzNDU2Nzg5MA==", result->size) == 0);
    freeBlock(result);

    input = resetBlock(input);
    addStringBlock(input, "\xf4\x74\xdb\xd0\x68\x26\xc9\x3e\x37\x32\xf9\x86\xe3\x33\x9e\x78\x25\x20\xb2\x28", 20);
    result = base64_encode(input);
    assert(memcmp(result->data, "9HTb0GgmyT43MvmG4zOeeCUgsig=", result->size) == 0);
    freeBlock(result);

    input = resetBlock(input);
    addStringBlock(input, "123456789", 9);
    result = base64_encode(input);
    assert(memcmp(result->data, "MTIzNDU2Nzg5", result->size) == 0);
    freeBlock(result);

    input = resetBlock(input);
    addStringBlock(input, "12345678", 8);
    result = base64_encode(input);
    assert(memcmp(result->data, "MTIzNDU2Nzg=", result->size) == 0);
    freeBlock(result);

    input = resetBlock(input);
    addStringBlock(input, "1", 1);
    result = base64_encode(input);
    assert(memcmp(result->data, "MQ==", result->size) == 0);
    freeBlock(result);

    input = resetBlock(input);
    addStringBlock(input, "12", 2);
    result = base64_encode(input);
    assert(memcmp(result->data, "MTI=", result->size) == 0);
    freeBlock(result);

    input = resetBlock(input);
    addStringBlock(input, "123", 3);
    result = base64_encode(input);
    assert(memcmp(result->data, "MTIz", result->size) == 0);
    freeBlock(result);


    unsigned char buff[256];
    for (int i = 0; i < 256; ++i) {
        buff[i] = i;
    }
    input = resetBlock(input);
    addStringBlock(input, buff, 256);
    result = base64_encode(input);
    assert(memcmp(result->data,
                  "AAECAwQFBgcICQoLDA0ODxAREhMUFRYXGBkaGxwdHh8gISIjJCUmJygpKissLS4vMDEyMzQ1Njc4OTo7PD0+P0BBQkNERUZH"
                  "SElKS0xNTk9QUVJTVFVWV1hZWltcXV5fYGFiY2RlZmdoaWprbG1ub3BxcnN0dXZ3eHl6e3x9fn+AgYKDhIWGh4iJiouMjY6P"
                  "kJGSk5SVlpeYmZqbnJ2en6ChoqOkpaanqKmqq6ytrq+wsbKztLW2t7i5uru8vb6/wMHCw8TFxsfIycrLzM3Oz9DR0tPU1dbX"
                  "2Nna29zd3t/g4eLj5OXm5+jp6uvs7e7v8PHy8/T19vf4+fr7/P3+/w==",
                  result->size) == 0);
    freeBlock(result);

    for (int i = 0; i < 256; ++i) {
        buff[i] = 0;
    }
    input = resetBlock(input);
    addStringBlock(input, buff, 256);
    result = base64_encode(input);
    assert(memcmp(result->data,
                  "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
                  "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
                  "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
                  "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA==",
                  result->size) == 0);
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

    return LIST_BREAK_RETURN;
}

unsigned char listPrintCallback(struct list *list, struct item *item, void *args) {
    printf("f");

    return LIST_CONTINUE_RETURN;
}

int testList() {
    struct block *block = NULL;
    struct c_countChanges *c_countChanges = c_result();
/*
    printf("M:%llu C:%llu F:%llu T:%llu\n", c_countChanges->countMalloc, c_countChanges->countCalloc,
           c_countChanges->countFree,
           c_countChanges->countMalloc + c_countChanges->countCalloc - c_countChanges->countFree);
*/

    struct list *list = NULL;
    list = initList(NULL, 2, LIST_STARTING_NEST, 2, LIST_SEMAPHORE_ENABLE_LEAF, LITTLE_ENDIAN);

    // Обхожу пустой список
    mapList(list, NULL, &listPrintCallback);

    block = resetBlock(block);
    printList(list, 0, block, 1);
    // printf("%s\n", block->data);
    assert(memcmp(block->data, "List:\n", block->size) == 0);

    const unsigned char addCount = 17;
    unsigned char add[17][3] = {
            "14",
            "22",
            "12",
            "25",
            "13",
            "21",
            "11",
            "23",
            "10",
            "24",
            "15",
            "26",
            "16",
            "20",
            "30",
            "30",
            "30",
    };
    for (int index = 0; index < addCount; ++index) {
        // printf("Add %*s\n", 2, add[index]);
        setHash(list, add[index]);

        block = resetBlock(block);
        printList(list, 0, block, 0);
        // printf("%s\n", block->data);
    }

    block = resetBlock(block);
    printList(list, 0, block, 1);
    // printf("%s\n", block->data);
    assert(memcmp(block->data, "List:10 , 11 , 12 , 13 , 14 , 15 , 16 , 20 , 21 , 22 , 23 , 24 , 25 , 26 , 30 , \n",
                  block->size) == 0);

    // printf("reInitList\n");
    list = reInitList(list, 1);

    block = resetBlock(block);
    printList(list, 0, block, 1);
    // printf("%s\n", block->data);
    assert(memcmp(block->data, "List:10 11 12 13 14 15 16 , 20 21 22 23 24 25 26 , 30 , \n", block->size) == 0);

    const unsigned char deleteCount = 17;
    unsigned char delete[17][3] = {
            "14",
            "22",
            "12",
            "25",
            "13",
            "21",
            "11",
            "23",
            "10",
            "24",
            "15",
            "26",
            "16",
            "20",
            "30",
            "30",
            "30",
    };
    for (int index = 0; index < deleteCount; ++index) {
        // printf("Delete %*s\n", 2, delete[index]);
        deleteHash(list, delete[index]);

        block = resetBlock(block);
        printList(list, 0, block, 0);
        // printf("%s\n", block->data);
    }

    block = resetBlock(block);
    printList(list, 0, block, 1);
    // printf("%s\n", block->data);
    assert(memcmp(block->data, "List:\n", block->size) == 0);

    const unsigned char addCount2 = 2;
    unsigned char add2[2][3] = {
            "30",
            "31",
    };
    for (int index = 0; index < addCount2; ++index) {
        // printf("Add %*s\n", 2, add2[index]);
        setHash(list, add2[index]);

        block = resetBlock(block);
        printList(list, 0, block, 0);
        // printf("%s\n", block->data);
    }

    block = resetBlock(block);
    printList(list, 0, block, 1);
    // printf("%s\n", block->data);
    assert(memcmp(block->data, "List:30 31 , \n", block->size) == 0);

    // printf("Test callback (delete 30)\n");
    mapList(list, NULL, &listCallback);
    block = resetBlock(block);
    printList(list, 0, block, 1);
    // printf("%s\n", block->data);
    assert(memcmp(block->data, "List:31 , \n", block->size) == 0);

    // printf("Test callback (delete 31)\n");
    mapList(list, NULL, &listCallback);
    block = resetBlock(block);
    printList(list, 0, block, 1);
    // printf("%s\n", block->data);
    assert(memcmp(block->data, "List:\n", block->size) == 0);

    freeBlock(block);

/*
    printf("M:%llu C:%llu F:%llu T:%llu\n", c_countChanges->countMalloc, c_countChanges->countCalloc,
           c_countChanges->countFree,
           c_countChanges->countMalloc + c_countChanges->countCalloc - c_countChanges->countFree);
*/

    freeList(list, 1);

/*
    printf("M:%llu C:%llu F:%llu T:%llu\n", c_countChanges->countMalloc, c_countChanges->countCalloc,
           c_countChanges->countFree,
           c_countChanges->countMalloc + c_countChanges->countCalloc - c_countChanges->countFree);
*/

    assert(c_countChanges->countMalloc + c_countChanges->countCalloc - c_countChanges->countFree == 0);
/*
    if (c_countChanges->countMalloc + c_countChanges->countCalloc - c_countChanges->countFree) {
        printf("Not all allocate are free – failure\n");
    }
*/

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

    assert(memcmp(block->data, "12" "\x00" "4512345123451234567890678906789067890", block->size) == 0);

    freeBlock(block);

    return 1;
}
