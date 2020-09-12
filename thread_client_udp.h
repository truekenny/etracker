#ifndef SC6_THREAD_CLIENT_UDP_H
#define SC6_THREAD_CLIENT_UDP_H

#include <arpa/inet.h>

struct clientUdpArgs {
    int serverSocket;
    struct sockaddr_in * clientAddr;
    unsigned int transaction_id;

    unsigned long receiveCount;
    // struct rk_sema *sem;
    // struct queue **first;
    struct firstByteData *firstByteData;
    struct stats *stats;
    struct query *query;
    struct block *hashes;

    unsigned int *interval;
};

void *clientUdpHandler(void *);

void *clientUdpScrapeHandler(void *);

#endif //SC6_THREAD_CLIENT_UDP_H
