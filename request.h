#ifndef SC6_REQUEST_H
#define SC6_REQUEST_H

#include "block.h"

struct request {
    struct block *block;
    int socket;

    struct request *next;
};

void addRequest(struct request **firstRequest, struct request **lastRequest, int socket, struct block *block);

struct request *pullRequest(struct request **firstRequest, struct request **lastRequest);

void freeRequest(struct request *request);

#endif //SC6_REQUEST_H
