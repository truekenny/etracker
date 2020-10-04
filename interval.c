#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "interval.h"

void updateInterval(struct block *block, _Atomic(unsigned int) *interval,
        unsigned int minInterval, unsigned int maxInterval) {
    double load[3];

    if (getloadavg(load, 3) == -1) {
        return;
    }

    double maxAllowLoadAverage = ((double) sysconf(_SC_NPROCESSORS_ONLN)) - 0.5;

    unsigned int localInterval = *interval + ((load[2] < maxAllowLoadAverage) ? (-STEP_INTERVAL) : STEP_INTERVAL);
    localInterval = (localInterval > maxInterval) ? maxInterval : localInterval;
    localInterval = (localInterval < minInterval) ? minInterval : localInterval;

    if (block != NULL)
        addFormatStringBlock(block, 1000,
                             "LA: %.2f %.2f %.2f  ML: %.2f  I: %4d->%4d s",
                             load[0], load[1], load[2], maxAllowLoadAverage, *interval, localInterval);

    *interval = localInterval;
}
