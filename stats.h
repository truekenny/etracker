#ifndef SC6_STATS_H
#define SC6_STATS_H

#include <time.h>
#include "block.h"

struct stats {
    time_t time;
    unsigned int http_200;
    unsigned int http_400;
    unsigned int http_403;
    unsigned int http_404;

    unsigned int send_failed;
    unsigned int recv_failed;
    unsigned int accept_failed;

    unsigned int send_pass;
    unsigned int recv_pass;
    unsigned int accept_pass;

    unsigned int send_failed_udp;
    unsigned int recv_failed_udp;

    unsigned int send_pass_udp;
    unsigned int recv_pass_udp;

    unsigned int keep_alive;
    unsigned int no_keep_alive;

    unsigned long long recv_bytes;
    unsigned long long sent_bytes;

    unsigned long long recv_bytes_udp;
    unsigned long long sent_bytes_udp;

    unsigned int announce;
    unsigned int scrape;

    unsigned int connect_udp;
    unsigned int announce_udp;
    unsigned int scrape_udp;

    unsigned int recv_failed_failed_errno[256];
};

void formatStats(int threadNumber, struct block *block, struct stats *stats);

#endif //SC6_STATS_H
