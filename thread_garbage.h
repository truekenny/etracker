#ifndef SC6_THREAD_GARBAGE_H
#define SC6_THREAD_GARBAGE_H

#include "data_change.h"
#include "socket_garbage.h"

void runGarbageDataThread(struct firstByteData *firstByte);

void runGarbageSocketPoolThread(struct socketPool ** socketPool, struct rk_sema *semaphoreSocketPool);

#endif //SC6_THREAD_GARBAGE_H
