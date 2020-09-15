#ifndef SC6_THREAD_GARBAGE_H
#define SC6_THREAD_GARBAGE_H

#include "data_change.h"
#include "socket_garbage.h"
#include "stats.h"
#include "list.h"

void run15MinutesThread(struct firstByteData *firstByte, unsigned int *interval, struct rps *rps);

void runGarbageSocketPoolThread(struct list *socketList, struct stats *stats);

#endif //SC6_THREAD_GARBAGE_H
