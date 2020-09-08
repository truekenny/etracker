#include <stddef.h>
#include <stdio.h>
#include "request.h"
#include "alloc.h"

#define DEBUG_KQUEUE 0

void addRequest(struct request **firstRequest, struct request **lastRequest, int socket, struct block *block) {
    struct request *newLastRequest = c_calloc(1, sizeof(struct request));
    newLastRequest->socket = socket;
    newLastRequest->block = block;

    if (*lastRequest == NULL) {
        DEBUG_KQUEUE && printf("request.c: Last NULL %d\n", socket);

        *firstRequest = *lastRequest = newLastRequest;
    } else {
        DEBUG_KQUEUE && printf("request.c: Add %d\n", socket);

        (*lastRequest)->next = newLastRequest;
        *lastRequest = newLastRequest;
    }
}

struct request *pullRequest(struct request **firstRequest, struct request **lastRequest) {
    if (*firstRequest != NULL) {
        struct request *result = *firstRequest;

        *firstRequest = (*firstRequest)->next;

        if ((*firstRequest) == NULL) {
            *lastRequest = NULL;
        }

        return result;
    }

    return NULL;
}

void freeRequest(struct request *request) {
    DEBUG_KQUEUE && printf("request.c: freeConnection %d\n", request->socket);

    freeBlock(request->block);
    c_free(request);
}
