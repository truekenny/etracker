#include <stddef.h>
#include <stdio.h>
#include "udp_request.h"
#include "alloc.h"

void addUdpRequest(struct udpRequest **firstRequest, struct udpRequest **lastRequest, struct sockaddr_in6 clientAddr,
                   struct block *block, unsigned int receiveCount) {
    struct udpRequest *newLastRequest = c_calloc(1, sizeof(struct udpRequest));
    newLastRequest->block = block;
    newLastRequest->clientAddr = clientAddr;
    newLastRequest->receiveCount = receiveCount;

    if (*lastRequest == NULL) {
        *firstRequest = *lastRequest = newLastRequest;
    } else {
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
    freeBlock(request->block);
    c_free(request);
}
