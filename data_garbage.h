#ifndef SC6_DATA_GARBAGE_H
#define SC6_DATA_GARBAGE_H

#include "data_structure.h"
#include "block.h"
#include "list.h"

#define DATA_GARBAGE_MAX_ALIVE_TIME 1800

void runGarbageCollectorL(struct block *block, struct list *torrentList);

#endif //SC6_DATA_GARBAGE_H
