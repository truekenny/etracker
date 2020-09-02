#include <sys/time.h>
#include <stddef.h>
#include "time.h"

unsigned long getStartTime() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec) * 1000000 + (tv.tv_usec);
}

unsigned long getDiffTime(unsigned long startTime) {
    return getStartTime() - startTime;
}
