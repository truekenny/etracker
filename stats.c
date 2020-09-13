#include <stdlib.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>
#include "stats.h"
#include "alloc.h"

void formatStats(int threadNumber, struct block *block, struct stats *stats, unsigned int interval) {
    struct c_countChanges *countChanges = c_result();
    double load[3] = {0};

    getloadavg(load, 3);

    struct rlimit rlimit;

    getrlimit(RLIMIT_NOFILE, &rlimit);

    addFormatStringBlock(block, 4500,
                         "start_time = %.24s\n" "thread_number = %d\n\n"

                         "Load Avg = %.2f %.2f %.2f\n"
                         "Interval = %d\n"
                         "Active sockets: %d (rlimit %lld/%lld)\n\n"

                         "Malloc = %d\n"
                         "Calloc = %d\n"
                         "*alloc = %d\n"
                         "free = %d\n"
                         "*alloc - free = %d\n\n"

                         "stats.http_200 = %d\n"
                         "stats.http_400 = %d\n"
                         "stats.http_403 = %d (Full Scrape)\n"
                         "stats.http_404 = %d\n"
                         "stats.http_405 = %d (Not GET)\n"
                         "stats.http_408 = %d (Timeout)\n"
                         "stats.http_413 = %d (Oversize)\n\n"

                         "stats.close_pass = %d\n"
                         "stats.send_pass = %d\n"
                         "stats.recv_pass = %d\n"
                         "stats.accept_pass = %d\n\n"

                         "stats.close_failed = %d\n"
                         "stats.send_failed = %d\n"
                         "stats.recv_failed = %d\n"
                         "stats.accept_failed = %d\n\n"

                         "stats.send_pass_udp = %d\n" "stats.recv_pass_udp = %d\n\n"

                         "stats.send_failed_udp = %d\n" "stats.recv_failed_udp = %d\n\n"

                         "stats.keep_alive = %d\n" "stats.no_keep_alive = %d\n\n"

                         "stats.sent_bytes = %llu\n" "stats.recv_bytes = %llu\n\n"

                         "stats.sent_bytes_udp = %llu\n" "stats.recv_bytes_udp = %llu\n\n"

                         "stats.announce = %d\n" "stats.scrape = %d\n\n"

                         "stats.connect_udp = %d\n" "stats.announce_udp = %d\n" "stats.scrape_udp = %d\n\n",
                         ctime(&stats->time), threadNumber,

                         load[0], load[1], load[2],
                         interval,
                         stats->accept_pass - stats->close_pass - stats->close_failed,
                         rlimit.rlim_cur, rlimit.rlim_max,

                         countChanges->countMalloc,
                         countChanges->countCalloc,
                         countChanges->countMalloc + countChanges->countCalloc,
                         countChanges->countFree,
                         countChanges->countMalloc + countChanges->countCalloc - countChanges->countFree,

                         stats->http_200,
                         stats->http_400,
                         stats->http_403,
                         stats->http_404,
                         stats->http_405,
                         stats->http_408,
                         stats->http_413,

                         stats->close_pass,
                         stats->send_pass,
                         stats->recv_pass,
                         stats->accept_pass,

                         stats->close_failed,
                         stats->send_failed,
                         stats->recv_failed,
                         stats->accept_failed,

                         stats->send_pass_udp, stats->recv_pass_udp,

                         stats->send_failed_udp, stats->recv_failed_udp,

                         stats->keep_alive, stats->no_keep_alive,

                         stats->sent_bytes, stats->recv_bytes,

                         stats->sent_bytes_udp, stats->recv_bytes_udp,

                         stats->announce, stats->scrape,

                         stats->connect_udp, stats->announce_udp, stats->scrape_udp
    );
}
