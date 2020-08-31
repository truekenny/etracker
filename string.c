#include <string.h>
#include "string.h"

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

