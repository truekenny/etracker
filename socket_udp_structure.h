#ifndef SC6_SOCKET_UDP_STRUCTURE_H
#define SC6_SOCKET_UDP_STRUCTURE_H

#include "uri.h"

#define SOCKET_UDP_STRUCTURE_ACTION_CONNECT  0
#define SOCKET_UDP_STRUCTURE_ACTION_ANNOUNCE 1
#define SOCKET_UDP_STRUCTURE_ACTION_SCRAPE   2
#define SOCKET_UDP_STRUCTURE_ACTION_ERROR    3

#define SOCKET_UDP_STRUCTURE_PROTOCOL_ID 0x8019102717040000

/*
0       64-bit integer  protocol_id     0x41727101980 // magic constant
8       32-bit integer  action          0 // connect
12      32-bit integer  transaction_id
16
 */

struct connectRequest {
    unsigned long protocol_id;
    unsigned int action;
    unsigned int transaction_id;
} __attribute__((packed)); // 16

/*
0       32-bit integer  action          0 // connect
4       32-bit integer  transaction_id
8       64-bit integer  connection_id
16
 */

struct connectResponse {
    unsigned int action;
    unsigned int transaction_id;
    unsigned long connection_id;
} __attribute__((packed)); // 16

/*
0       64-bit integer  connection_id
8       32-bit integer  action          1 // announce
12      32-bit integer  transaction_id
16      20-byte string  info_hash
36      20-byte string  peer_id
56      64-bit integer  downloaded
64      64-bit integer  left
72      64-bit integer  uploaded
80      32-bit integer  event           0 // 0: none; 1: completed; 2: started; 3: stopped
84      32-bit integer  IP address      0 // default
88      32-bit integer  key
92      32-bit integer  num_want        -1 // default
96      16-bit integer  port
98
 */

struct announceRequest {
    unsigned long connection_id;
    unsigned int action;
    unsigned int transaction_id;
    unsigned char info_hash[URI_PARAM_VALUE_LENGTH];
    unsigned char peer_id[URI_PARAM_VALUE_LENGTH];
    unsigned long downloaded;
    unsigned long left;
    unsigned long uploaded;
    unsigned int event;
    unsigned int ip;
    unsigned int key;
    unsigned int num_want;
    unsigned short port;
} __attribute__((packed)); // 98

/*
0               64-bit integer  connection_id
8               32-bit integer  action          2 // scrape
12              32-bit integer  transaction_id
16
 */

struct scrapeRequest {
    unsigned long connection_id;
    unsigned int action;
    unsigned int transaction_id;
} __attribute__ ((packed)); // 16

/*
0           32-bit integer  action          1 // announce
4           32-bit integer  transaction_id
8           32-bit integer  interval
12          32-bit integer  leechers
16          32-bit integer  seeders
20 + 6 * n  32-bit integer  IP address
24 + 6 * n  16-bit integer  TCP port
26
 */

struct announceHeadResponse {
    unsigned int action;
    unsigned int transaction_id;
    unsigned int interval;
    unsigned int leechers;
    unsigned int seeders;
} __attribute__((packed)); // 20

struct announcePeerResponse {
    unsigned int ip;
    unsigned short port;
} __attribute__((packed)); // 6

/*
0           32-bit integer  action          2 // scrape
4           32-bit integer  transaction_id
8
 */

struct scrapeHeadResponse {
    unsigned int action;
    unsigned int transaction_id;
} __attribute__ ((packed)); // 8

/*
0           32-bit integer  seeders
4           32-bit integer  completed
8           32-bit integer  leechers
12
 */

struct scrapeTorrentResponse {
    unsigned int seeders;
    unsigned int completed;
    unsigned int leechers;
};

#endif //SC6_SOCKET_UDP_STRUCTURE_H
