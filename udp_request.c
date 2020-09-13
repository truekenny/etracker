#include <stddef.h>
#include <stdio.h>
#include "udp_request.h"
#include "alloc.h"

#define DEBUG_UDP 0

void addUdpRequest(struct udpRequest **firstRequest, struct udpRequest **lastRequest, struct sockaddr_in *clientAddr,
                   struct block *block, unsigned int receiveCount) {
    struct udpRequest *newLastRequest = c_calloc(1, sizeof(struct udpRequest));
    newLastRequest->block = block;
    newLastRequest->clientAddr = clientAddr;
    newLastRequest->receiveCount = receiveCount;

    if (*lastRequest == NULL) {
        DEBUG_UDP && printf("request.c: Last NULL %d\n", -1);

        *firstRequest = *lastRequest = newLastRequest;
    } else {
        DEBUG_UDP && printf("request.c: Add %d\n", -1);

        (*lastRequest)->next = newLastRequest;
        *lastRequest = newLastRequest;
    }
}

struct udpRequest *pullUdpRequest(struct udpRequest **firstRequest, struct udpRequest **lastRequest) {
    if (*firstRequest != NULL) {
        struct udpRequest *result = *firstRequest;

        *firstRequest = (*firstRequest)->next;

        if ((*firstRequest) == NULL) {
            *lastRequest = NULL;
        }

        return result;
    }

    return NULL;
}

void freeUdpRequest(struct udpRequest *request) {
    DEBUG_UDP && printf("request.c: freeConnection %d\n", -1);

    freeBlock(request->block);
    c_free(request->clientAddr);
    c_free(request);
}
