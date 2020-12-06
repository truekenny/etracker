#ifndef SC6_STATS_H
#define SC6_STATS_H

#include <time.h>
#include <stdatomic.h>
#include "block.h"
#include "rps.h"
#include "interval.h"

#define STATS_ERRNO_MAX_INDEX 256
// Считать, что обновление пира, которое произошло позднее 3667 секунд, считается новым пиром
#define STATS_MAX_DELAY_BETWEEN_UPDATE_S 3667

struct stats {
    time_t time;
    int failed;
    atomic_ullong http_101;
    atomic_ullong http_200;
    atomic_ullong http_400;
    atomic_ullong http_401;
    atomic_ullong http_403;
    atomic_ullong http_404;
    atomic_ullong http_405;
    atomic_ullong http_408;
    atomic_ullong http_413;
    atomic_ullong http_507;

    atomic_ullong close_failed;
    atomic_ullong send_skips;
    atomic_ullong send_failed;
    atomic_ullong recv_failed;
    atomic_ullong recv_failed_read_0;
    atomic_ullong recv_failed_read_sub_0;
    atomic_ullong recv_failed_read_not_equal;
    atomic_ullong accept_failed;

    atomic_ullong close_pass;
    atomic_ullong send_pass;
    atomic_ullong recv_pass;
    atomic_ullong accept_pass;

    atomic_ullong send_failed_udp;
    atomic_ullong recv_failed_udp;

    atomic_ullong send_pass_udp;
    atomic_ullong recv_pass_udp;

    atomic_ullong keep_alive;
    atomic_ullong no_keep_alive;

    atomic_ullong recv_bytes;
    atomic_ullong sent_bytes;

    atomic_ullong recv_bytes_udp;
    atomic_ullong sent_bytes_udp;

    atomic_ullong announce;
    atomic_ullong scrape;

    atomic_ullong connect_udp;
    atomic_ullong announce_udp;
    atomic_ullong scrape_udp;

    atomic_ullong close_errno[STATS_ERRNO_MAX_INDEX + 1];
    atomic_ullong send_errno[STATS_ERRNO_MAX_INDEX + 1];
    atomic_ullong recv_errno[STATS_ERRNO_MAX_INDEX + 1];
    atomic_ullong accept_errno[STATS_ERRNO_MAX_INDEX + 1];

    atomic_ullong send_errno_udp[STATS_ERRNO_MAX_INDEX + 1];
    atomic_ullong recv_errno_udp[STATS_ERRNO_MAX_INDEX + 1];

    atomic_ullong update_peer[STATS_MAX_DELAY_BETWEEN_UPDATE_S + 1];
};

void
formatStats(int threadNumber, struct block *block, struct stats *stats, struct interval *interval, struct rps *rps);

void incErrno(atomic_ullong *statErrno);

void updatePeerStat(struct stats *stats, unsigned int delay);

#endif //SC6_STATS_H
