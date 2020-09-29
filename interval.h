#ifndef SC6_INTERVAL_H
#define SC6_INTERVAL_H

#include <stdatomic.h>
#include "block.h"

#define MAX_INTERVAL 1799
#define STEP_INTERVAL 60

void updateInterval(struct block *block, _Atomic(unsigned int) *interval);

#endif //SC6_INTERVAL_H
