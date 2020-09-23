#ifndef SC6_INTERVAL_H
#define SC6_INTERVAL_H

#include "block.h"

#define MAX_INTERVAL 1799
#define STEP_INTERVAL 60

void updateInterval(struct block *block, unsigned int *interval);

#endif //SC6_INTERVAL_H
