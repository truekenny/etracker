#ifndef SC6_DATA_GARBAGE_H
#define SC6_DATA_GARBAGE_H

#include "data_structure.h"
#include "block.h"
#include "list.h"

void runGarbageCollectorL(struct block *block, struct list *torrentList, unsigned int aliveTime);

#endif //SC6_DATA_GARBAGE_H
