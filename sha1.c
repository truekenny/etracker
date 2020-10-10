#include <string.h>
#include <iso646.h>
#include <arpa/inet.h>
#include "sha1.h"
#include "socket.h"

// @see https://ru.wikipedia.org/wiki/SHA-1

unsigned int leftRotate(unsigned int input, unsigned char rotate) {
    return input << rotate | input >> (32u - rotate);
}

void sha1(struct block *block) {
    unsigned long long size = htonll((unsigned long long) block->size * 8); // 1 byte = 8 bits
    unsigned char one = 0b10000000;
    unsigned char zero[64] = {0};

    unsigned int h[5];
    h[0] = 0x67452301;
    h[1] = 0xEFCDAB89;
    h[2] = 0x98BADCFE;
    h[3] = 0x10325476;
    h[4] = 0xC3D2E1F0;

    addStringBlock(block, &one, 1);
    // 512 = 64
    // 448 = 56
    unsigned char left = block->size % 64;
    if (left < 56) {
        // дополнить до 56
        unsigned char zeroDataLength = 56 - left;
        addStringBlock(block, zero, zeroDataLength);
    } else if (left > 56) {
        // дополнить до 64
        unsigned char zeroDataLength = 64 - left;
        addStringBlock(block, zero, zeroDataLength);
        // и добавить 56
        addStringBlock(block, zero, 56);
    }
    addStringBlock(block, &size, 8);

    for (unsigned int index = 0; index < block->size / 64; ++index) {
        unsigned int w[80];
        //memcpy(&w, &block->data[index * 64], 64);
        for (int i = 0; i < 16; ++i) {
            w[i] = htonl(*(unsigned int *) &block->data[index * 64 + i * 4]);
        }

        for (int i = 16; i < 80; ++i) {
            w[i] = leftRotate(w[i - 3] xor w[i - 8] xor w[i - 14] xor w[i - 16], 1);
        }

        unsigned int a = h[0];
        unsigned int b = h[1];
        unsigned int c = h[2];
        unsigned int d = h[3];
        unsigned int e = h[4];
        unsigned int f;
        unsigned int k;

        for (int i = 0; i < 80; ++i) {
            if (i < 20) {
                f = (b bitand c) bitor ((compl b) bitand d);
                k = 0x5A827999;
            } else if (i < 40) {
                f = b xor c xor d;
                k = 0x6ED9EBA1;
            } else if (i < 60) {
                f = (b bitand c) bitor (b bitand d) bitor (c bitand d);
                k = 0x8F1BBCDC;
            } else /*if (i < 80)*/ {
                f = b xor c xor d;
                k = 0xCA62C1D6;
            }

            unsigned int temp = leftRotate(a, 5) + f + e + k + w[i];
            e = d;
            d = c;
            c = leftRotate(b, 30);
            b = a;
            a = temp;
        }

        h[0] = h[0] + a;
        h[1] = h[1] + b;
        h[2] = h[2] + c;
        h[3] = h[3] + d;
        h[4] = h[4] + e;
    }

    block = resetBlock(block);
    for (int indexH = 0; indexH < 5; ++indexH) {
        h[indexH] = htonl(h[indexH]);
        addStringBlock(block, &h[indexH], 4);
    }
}
