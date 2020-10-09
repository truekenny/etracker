#ifndef SC6_STATS_H
#define SC6_STATS_H

#include <time.h>
#include <stdatomic.h>
#include "block.h"
#include "rps.h"
#include "interval.h"

struct stats {
    time_t time;
    int failed;
    atomic_uint http_200;
    atomic_uint http_400;
    atomic_uint http_401;
    atomic_uint http_403;
    atomic_uint http_404;
    atomic_uint http_405;
    atomic_uint http_408;
    atomic_uint http_413;
    atomic_uint http_507;

    atomic_uint close_failed;
    atomic_uint send_failed;
    atomic_uint recv_failed;
    atomic_uint recv_failed_read_0;
    atomic_uint recv_failed_read_sub_0;
    atomic_uint recv_failed_read_not_equal;
    atomic_uint accept_failed;

    atomic_uint close_pass;
    atomic_uint send_pass;
    atomic_uint recv_pass;
    atomic_uint accept_pass;

    atomic_uint send_failed_udp;
    atomic_uint recv_failed_udp;

    atomic_uint send_pass_udp;
    atomic_uint recv_pass_udp;

    atomic_uint keep_alive;
    atomic_uint no_keep_alive;

    atomic_ullong recv_bytes;
    atomic_ullong sent_bytes;

    atomic_ullong recv_bytes_udp;
    atomic_ullong sent_bytes_udp;

    atomic_uint announce;
    atomic_uint scrape;

    atomic_uint connect_udp;
    atomic_uint announce_udp;
    atomic_uint scrape_udp;
};

void
formatStats(int threadNumber, struct block *block, struct stats *stats, struct interval *interval, struct rps *rps);

#endif //SC6_STATS_H
