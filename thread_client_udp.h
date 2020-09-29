#ifndef SC6_THREAD_CLIENT_UDP_H
#define SC6_THREAD_CLIENT_UDP_H

#include <stdatomic.h>
#include <arpa/inet.h>
#include "udp_request.h"
#include "rps.h"

struct clientUdpArgs {
    int serverSocket;
    struct list *torrentList;
    struct stats *stats;
    _Atomic(unsigned int) *interval;

    unsigned int threadNumber;

    pthread_cond_t *signalRequest;
    pthread_mutex_t *mutexSignalRequest;
    struct udpRequest **firstRequest;
    struct udpRequest **lastRequest;
    struct rk_sema *semaphoreRequest;

    struct rps *rps;
    unsigned int *maxPeersPerResponse;
};

void *clientUdpHandler(void *);

#endif //SC6_THREAD_CLIENT_UDP_H
