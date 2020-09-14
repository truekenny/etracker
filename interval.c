#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "interval.h"

#define MIN_INTERVAL 239
#define STEP_INTERVAL 60

void updateInterval(struct block *block, unsigned int *interval) {
    double load[3];

    if (getloadavg(load, 3) == -1) {
        return;
    }

    double maxAllowLoadAverage = ((double) sysconf(_SC_NPROCESSORS_ONLN)) - 0.5;

    unsigned int localInterval = *interval + ((load[2] < maxAllowLoadAverage) ? (-STEP_INTERVAL) : STEP_INTERVAL);
    localInterval = (localInterval > MAX_INTERVAL) ? MAX_INTERVAL : localInterval;
    localInterval = (localInterval < MIN_INTERVAL) ? MIN_INTERVAL : localInterval;

    if (block != NULL)
        addFormatStringBlock(block, 1000,
                             "LA: %.2f %.2f %.2f  ML: %.2f  I: %4d->%4d s",
                             load[0], load[1], load[2], maxAllowLoadAverage, *interval, localInterval);

    *interval = localInterval;
}
