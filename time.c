#include <sys/time.h>
#include "time.h"

double getStartTime() {
    struct timeval  tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec) * 1000 + (tv.tv_usec) / 1000 ;
}

double getDiffTime(double startTime) {
    return getStartTime() - startTime;
}
