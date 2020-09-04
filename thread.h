#ifndef SC6_THREAD_H
#define SC6_THREAD_H

#include "data.h"
#include "stats.h"

/**
 * Аргументы переданные в поток
 */
struct args {
    int sock;
    int number;
    struct rk_sema *sem;
    struct queue **first;
    in_addr_t ip;
    struct firstByte *firstByte;
    struct stats *stats;
};

void runGarbageCollectorThread(struct firstByte *firstByte);

void *connection_handler(void *);

#endif //SC6_THREAD_H
