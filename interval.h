#ifndef SC6_INTERVAL_H
#define SC6_INTERVAL_H

#include <stdatomic.h>
#include "block.h"

#define STEP_INTERVAL 60

void updateInterval(struct block *block, _Atomic(unsigned int) *interval,
        unsigned int minInterval, unsigned int maxInterval);

#endif //SC6_INTERVAL_H
