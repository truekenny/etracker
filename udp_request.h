#ifndef SC6_UDP_REQUEST_H
#define SC6_UDP_REQUEST_H

#include <arpa/inet.h>
#include "block.h"

struct udpRequest {
    struct block *block;
    struct sockaddr_in6 clientAddr;
    unsigned int receiveCount;

    struct udpRequest *next;
};

void addUdpRequest(struct udpRequest **firstRequest, struct udpRequest **lastRequest, struct sockaddr_in6 clientAddr,
                   struct block *block, unsigned int receiveCount);

struct udpRequest *pullUdpRequest(struct udpRequest **firstRequest, struct udpRequest **lastRequest);

void freeUdpRequest(struct udpRequest *request);

#endif //SC6_UDP_REQUEST_H
