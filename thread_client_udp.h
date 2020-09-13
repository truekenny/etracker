#ifndef SC6_THREAD_CLIENT_UDP_H
#define SC6_THREAD_CLIENT_UDP_H

#include <arpa/inet.h>
#include "udp_request.h"
#include "rps.h"

struct clientUdpArgs {
    int serverSocket;
    struct firstByteData *firstByteData;
    struct stats *stats;
    unsigned int *interval;

    unsigned int threadNumber;

    pthread_cond_t *signalRequest;
    pthread_mutex_t *mutexSignalRequest;
    struct udpRequest **firstRequest;
    struct udpRequest **lastRequest;
    struct rk_sema *semaphoreRequest;

    struct rps *rps;
};

void *clientUdpHandler(void *);

#endif //SC6_THREAD_CLIENT_UDP_H
