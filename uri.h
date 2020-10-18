#ifndef SC6_URI_H
#define SC6_URI_H

#include <arpa/inet.h>
#include "block.h"

#define URI_RANDOM_DATA_INFO_HASH 0
#define URI_RANDOM_DATA_PEER_ID   0

// При добавлении или удалении
#define URI_EVENT_ID_NONE      0
#define URI_EVENT_ID_COMPLETED 1
#define URI_EVENT_ID_STARTED   2
#define URI_EVENT_ID_STOPPED   3
#define URI_EVENT_ID_PAUSED    4
// Event не указан
#define URI_EVENT_ID_CONTINUE  5
#define URI_EVENT_STRING_COMPLETED "completed"
#define URI_EVENT_STRING_STARTED   "started"
#define URI_EVENT_STRING_STOPPED   "stopped"
#define URI_EVENT_STRING_PAUSED    "paused"
// Обязательно 20, очень большая связанность
#define URI_PARAM_VALUE_LENGTH 20
#define URI_DEFAULT_NUM_WANT 50
#define URI_DEFAULT_COMPACT 1u

#define URI_PATH_LENGTH 200

#define URI_QUERY_PROTOCOL_TCP 0
#define URI_QUERY_PROTOCOL_UDP 1

struct query {
    char path[URI_PATH_LENGTH + 1];
    unsigned char has_info_hash;
    unsigned char info_hash[URI_PARAM_VALUE_LENGTH];
    unsigned char event;
    unsigned short port;
    struct in6_addr ip;
    unsigned char peer_id[URI_PARAM_VALUE_LENGTH];
    unsigned char protocol;
    _Bool compact;
    _Bool no_peer_id;
    unsigned int numwant;

    int threadNumber;
    unsigned int transaction_id;

    // URI=/set
    unsigned int interval;
    unsigned int max_peers_per_response;
    unsigned short socket_timeout;
    unsigned char keep_alive;

    unsigned char ipVersion;
    char *xForwardedFor;
};

void parseUri(struct query *query, struct block *hashesBlock, struct block *forwardedForBlock, char *message);

#endif //SC6_URI_H
