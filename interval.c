#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include "interval.h"
#include "math.h"

unsigned int niceInterval(struct interval *interval, unsigned int value);

void updateInterval(struct block *block, struct interval *interval) {
    double load[3];

    if (getloadavg(load, 3) == -1) {
        printf("getloadavg failed: %d, %s\n", errno, strerror(errno));

        return;
    }

    double maxAllowLoadAverage = ((double) sysconf(_SC_NPROCESSORS_ONLN)) - 0.5;

    unsigned int localInterval = interval->requireInterval;

    if (load[0] < maxAllowLoadAverage && load[1] < maxAllowLoadAverage && load[2] < maxAllowLoadAverage)
        localInterval -= STEP_INTERVAL;
    else if (load[0] > maxAllowLoadAverage && load[1] > maxAllowLoadAverage && load[2] > maxAllowLoadAverage)
        localInterval += STEP_INTERVAL;

    localInterval = niceInterval(interval, localInterval);

    interval->previousInterval = interval->requireInterval;
    interval->requireInterval = localInterval;

    if (block != NULL)
        addFormatStringBlock(block, 1000,
                             "LA: %.2f %.2f %.2f  ML: %.2f  I: %d (%d->%d) s",
                             load[0], load[1], load[2], maxAllowLoadAverage,
                             interval->interval, interval->previousInterval, interval->requireInterval);
}

void forceUpdateInterval(struct interval *interval, unsigned int value) {
    value = niceInterval(interval, value);

    interval->interval = value;
    interval->requireInterval = value;
    interval->previousInterval = value;
}

/**
 * Изменяет фактический интервал
 * @param interval
 * @return Кол-во секунд для сна из учёта, что:
 *  за interval->previousInterval секунд интервал измениться на STEP_INTERVAL секунд
 */
unsigned int stepInterval(struct interval *interval) {
    if (interval->interval < interval->requireInterval)
        interval->interval++;
    else if (interval->interval > interval->requireInterval)
        interval->interval--;

    return max(round((double) interval->previousInterval / STEP_INTERVAL), 1u);
}

/**
 * @param interval
 * @param value
 * @return допустимый интервал
 */
unsigned int niceInterval(struct interval *interval, unsigned int value) {
    value = max(interval->minInterval, value);
    value = min(interval->maxInterval, value);

    return value;
}
