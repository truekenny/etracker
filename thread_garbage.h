#ifndef SC6_THREAD_GARBAGE_H
#define SC6_THREAD_GARBAGE_H

#include "socket_garbage.h"
#include "stats.h"
#include "list.h"
#include "interval.h"

void runIntervalChangerThread(struct interval *interval);

void runGarbageCollectorThread(struct list *torrentList, struct interval *interval, struct rps *rps, double maxLoadAvg);

void runGarbageSocketTimeoutThread(struct list **socketLists, struct stats *stats, unsigned short *socketTimeout,
        long workers, struct list *websockets);

#endif //SC6_THREAD_GARBAGE_H
