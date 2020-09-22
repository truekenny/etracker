#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <memory.h>
#include "basic.h"
#include "block.h"
#include "base64.h"

_Bool hasBasic(char *buffer, char *search) {
    return strstr(buffer, search) != NULL;
}

struct block *randomString(unsigned char size) {
    unsigned char symbols[63] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";

    struct block *result = initBlock();
    while (size > 0) {
        addStringBlock(result, &symbols[rand() % 62], 1);
        size--;
    }

    return result;
}

void getAuthorizationHeader(struct block *authorizationHeader) {
    srand(time(NULL));

    struct block *password = randomString(10);
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
}
