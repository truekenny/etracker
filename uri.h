#ifndef SC6_URI_H
#define SC6_URI_H

#include "block.h"

#define RANDOM_DATA_INFO_HASH 0
#define RANDOM_DATA_PEER_ID 0

// При добавлении или удалении
#define EVENT_ID_NONE      0
#define EVENT_ID_COMPLETED 1
#define EVENT_ID_STARTED   2
#define EVENT_ID_STOPPED   3
#define EVENT_ID_PAUSED    4
// Event не указан
#define EVENT_ID_CONTINUE  5
#define EVENT_STRING_COMPLETED "completed"
#define EVENT_STRING_STARTED   "started"
#define EVENT_STRING_STOPPED   "stopped"
#define EVENT_STRING_PAUSED    "paused"
// Обязательно 20, очень большая связанность
#define PARAM_VALUE_LENGTH 20
#define DEFAULT_NUM_WANT 50

struct query {
    unsigned char has_info_hash;
    unsigned char info_hash[PARAM_VALUE_LENGTH];
    unsigned char event;
    unsigned short port;
    unsigned int ip;
    unsigned char peer_id[PARAM_VALUE_LENGTH];
    _Bool udp;
    _Bool compact;
    _Bool no_peer_id;
    unsigned int numwant;

    int threadNumber;
    unsigned int transaction_id;
};

void parseUri(struct query *query, struct block *block, char *message);

#endif //SC6_URI_H
