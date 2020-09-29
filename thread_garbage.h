#ifndef SC6_THREAD_GARBAGE_H
#define SC6_THREAD_GARBAGE_H

#include "socket_garbage.h"
#include "stats.h"
#include "list.h"

void run15MinutesThread(struct list *torrentList, _Atomic(unsigned int) *interval, struct rps *rps);

void runGarbageSocketTimeoutThread(struct list **socketLists, struct stats *stats, unsigned short *socketTimeout, long workers);

#endif //SC6_THREAD_GARBAGE_H
