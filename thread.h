#ifndef SC6_THREAD_H
#define SC6_THREAD_H

#define _GNU_SOURCE
#include <pthread.h>

void pthreadSetName(pthread_t id, char *name);

#endif //SC6_THREAD_H
