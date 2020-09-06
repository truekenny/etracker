#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <math.h>
#include "string.h"
#include "alloc.h"

/**
 * @param char* start
 * @param char* string
 * @return _Bool Строка str начинается на pre
 */
_Bool startsWith(const char *start, const char *string) {
    size_t lenPre = strlen(start),
            lenStr = strlen(string);
    return lenStr < lenPre ? 0 : memcmp(start, string, lenPre) == 0;
}

void printHex(char *string, unsigned int len) {
    unsigned int ceilLen = (int) ceil((double) len / 16) * 16;

    printf("%4s | %2d %2d %2d %2d   %2d %2d %2d %2d   %2d %2d %2d %2d   %2d %2d %2d %2d | 0123 4567 8901 2345\n"
           "-----+-------------------------------------------------------+--------------------\n",
           " ", 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15);

    int index;
    for (index = 0; index < ceilLen; ++index) {
        if (index % 16 == 0) {
            printf("%4d | ", index);
        }

        if (index < len)
            printf("%02x ", string[index] & 0xff);
        else
            printf("   ");

        if (index % 4 == 3 && index % 16 != 15)
            printf("  ");
        if (index % 16 == 15) {

            printf("| ");

            for (int j = index - 15; j <= index; ++j) {
                if (j < len) {
                    if (isprint(string[j])) {
                        printf("%c", string[j]);
                    } else {
                        printf(".");
                    }

                    if (j % 4 == 3)
                        printf(" ");
                }
            }

            printf("\n");
        }
    }
}