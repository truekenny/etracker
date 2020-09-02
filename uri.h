#ifndef SC6_URI_H
#define SC6_URI_H

#define RANDOM_DATA_INFO_HASH 0
#define RANDOM_DATA_PEER_ID 1
#define EVENT_ID_STARTED   1
#define EVENT_ID_COMPLETED 2
#define EVENT_ID_STOPPED   3
#define EVENT_ID_PAUSED    4
#define EVENT_STRING_STARTED   "started"
#define EVENT_STRING_COMPLETED "completed"
#define EVENT_STRING_STOPPED   "stopped"
#define EVENT_STRING_PAUSED    "paused"
// Обязательно 20, очень большая связанность
#define PARAM_VALUE_LENGTH 20
#define DEFAULT_NUM_WANT 50

struct query {
    unsigned char info_hash[PARAM_VALUE_LENGTH];
    unsigned char event;
    unsigned short port;
    unsigned int ip;
    unsigned char peer_id[PARAM_VALUE_LENGTH];
    _Bool compact;
    _Bool no_peer_id;
    unsigned int numwant;
};

void parseUri(struct query *query, char *message);

#endif //SC6_URI_H
