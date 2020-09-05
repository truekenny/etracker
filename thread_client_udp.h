#ifndef SC6_THREAD_CLIENT_UDP_H
#define SC6_THREAD_CLIENT_UDP_H

#include <ntsid.h>
#include <arpa/inet.h>

struct clientUdpArgs {
    int serverSocket;
    struct sockaddr_in * clientAddr;
    unsigned int transaction_id;

    unsigned long number;
    struct rk_sema *sem;
    // struct queue **first;
    struct firstByte *firstByte;
    struct stats *stats;
    struct query *query;
};

void *clientUdpHandler(void *);

#endif //SC6_THREAD_CLIENT_UDP_H
