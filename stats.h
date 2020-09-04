#ifndef SC6_STATS_H
#define SC6_STATS_H

#include <time.h>

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
    unsigned int keep_alive;
    unsigned int no_keep_alive;
    unsigned long long recv_bytes;
    unsigned long long sent_bytes;

};

#endif //SC6_STATS_H
