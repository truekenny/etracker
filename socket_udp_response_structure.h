#ifndef SC6_SOCKET_UDP_RESPONSE_STRUCTURE_H
#define SC6_SOCKET_UDP_RESPONSE_STRUCTURE_H

#define ACTION_CONNECT 0
#define ACTION_ANNOUNCE 1

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

#endif //SC6_SOCKET_UDP_RESPONSE_STRUCTURE_H
