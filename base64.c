#include <math.h>
#include <stdlib.h>
#include "base64.h"

unsigned char getIndex(struct block *block, unsigned int index) {
    if (index >= block->size)
        return 0;

    return block->data[index];
}

struct block *base64_encode(struct block *input) {
    unsigned char endSymbolIndex = 64;
    unsigned char symbols[66] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=";

    struct block *result = initBlock();
    unsigned char *inputData = (unsigned char *) input->data;

    for (unsigned sourceBlockIndex = 0; sourceBlockIndex < ceil((double) input->size / 3); sourceBlockIndex++) {
        unsigned offset = sourceBlockIndex * 3;


        for (unsigned char resultIndex = 0; resultIndex < 4; resultIndex++) {
            unsigned char indexString = 0;

            switch (resultIndex) {
                case 0:
                    indexString = inputData[offset] >> 2;
                    break;
                case 1:
                    indexString = (inputData[offset] << 4 | getIndex(input, offset + 1) >> 4) & 0b111111;
                    break;
                case 2:
                    if (offset + 2 > input->size)
                        indexString = endSymbolIndex;
                    else
                        indexString = (getIndex(input, offset + 1) << 2 | getIndex(input, offset + 2) >> 6) & 0b111111;
                    break;
                case 3:
                    if (offset + 3 > input->size)
                        indexString = endSymbolIndex;
                    else
                        indexString = getIndex(input, offset + 2) & 0b111111;
                    break;
            }

            addStringBlock(result, &symbols[indexString], 1);
        }
    }

    return result;
}
