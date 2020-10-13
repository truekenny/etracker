#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <memory.h>
#include <errno.h>
#include "basic.h"
#include "block.h"
#include "base64.h"
#include "time.h"

#define BASIC_PASSWORD_LENGTH 20

_Bool hasBasic(char *buffer, char *search) {
    return strstr(buffer, search) != NULL;
}

void getRandom(unsigned char *randomBytes, unsigned int size) {
    FILE *file = fopen("/dev/urandom", "rb");

    if (file != NULL) {
        size_t readSize = fread(randomBytes, 1, size, file);

        if(readSize != size) {
            printf("basic.c: fread failed: %d: %s\n", errno, strerror(errno));
        }

        fclose(file);
    } else {
        printf("basic.c: fopen failed: %d: %s\n", errno, strerror(errno));
    }
}

struct block *randomString(unsigned char size) {
    char *sourceString = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    size_t sourceStringLength = strlen(sourceString);
    unsigned char randomBytes[size];
    getRandom(randomBytes, size);

    struct block *result = initBlock();
    while (size > 0) {
        addStringBlock(result, &sourceString[randomBytes[size - 1] % sourceStringLength], 1);
        size--;
    }

    return result;
}

void getAuthorizationHeader(struct block *authorizationHeader) {
    struct block *password = randomString(BASIC_PASSWORD_LENGTH);
    printf("Password: %.*s\n", password->size, password->data);
    struct block *base64_input = initBlock();
    addStringBlock(base64_input, "etracker:", 9);
    addStringBlock(base64_input, password->data, password->size);
    freeBlock(password);
    struct block *base64_encoded = base64_encode(base64_input);
    freeBlock(base64_input);
    addStringBlock(authorizationHeader, "Authorization: Basic ", 21);
    addStringBlock(authorizationHeader, base64_encoded->data, base64_encoded->size);
    addStringBlock(authorizationHeader, "\r\x00", 2);
    freeBlock(base64_encoded);
}
