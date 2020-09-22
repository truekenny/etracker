#ifndef SC6_THREAD_GARBAGE_H
#define SC6_THREAD_GARBAGE_H

#include "socket_garbage.h"
#include "stats.h"
#include "list.h"

void run15MinutesThread(struct list *torrentList, unsigned int *interval, struct rps *rps);

void runGarbageSocketTimeoutThread(struct list *socketList, struct stats *stats, unsigned short *socketTimeout);

#endif //SC6_THREAD_GARBAGE_H
