#ifndef SC6_URI_H
#define SC6_URI_H

#define EVENT_ID_STARTED   1
#define EVENT_ID_COMPLETED 2
#define EVENT_ID_STOPPED   3
#define EVENT_STRING_STARTED   "started"
#define EVENT_STRING_COMPLETED "completed"
#define EVENT_STRING_STOPPED   "stopped"
#define PARAM_VALUE_LENGTH 20

struct query {
    char info_hash[PARAM_VALUE_LENGTH];
    unsigned char event;
    unsigned short port;
    unsigned int ip;
    char peer_id[PARAM_VALUE_LENGTH];
    _Bool compact;
    _Bool no_peer_id;
};

void parseUri(struct query *query, char *message);

#endif //SC6_URI_H
