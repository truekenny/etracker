#ifndef SC6_INTERVAL_H
#define SC6_INTERVAL_H

#include <stdatomic.h>
#include "block.h"

#define INTERVAL_STEP_S 60

struct interval {
    _Atomic(unsigned int) interval;
    _Atomic(unsigned int) requireInterval;
    _Atomic(unsigned int) previousInterval;
    unsigned int maxInterval;
    unsigned int minInterval;
};

void updateInterval(struct block *block, struct interval*interval, double maxLoadAvg);

void forceUpdateInterval(struct interval *interval, unsigned int value);

unsigned int stepInterval(struct interval *interval);

#endif //SC6_INTERVAL_H
