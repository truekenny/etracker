#ifndef SC6_THREAD_GARBAGE_H
#define SC6_THREAD_GARBAGE_H

#include "data_change.h"
#include "socket_garbage.h"
#include "stats.h"

void run15MinutesThread(struct firstByteData *firstByte, unsigned int *interval, struct rps *rps);

void runGarbageSocketPoolThread(struct socketPool ** socketPool, struct rk_sema *semaphoreSocketPool, struct stats *stats);

#endif //SC6_THREAD_GARBAGE_H
