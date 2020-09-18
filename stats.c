#include <stdlib.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>
#include "stats.h"
#include "alloc.h"

#if !defined(REVISION)
#define REVISION "UNKNOWN"
#endif

void formatStats(int threadNumber, struct block *block, struct stats *stats, unsigned int interval, struct rps *rps) {
    struct c_countChanges *countChanges = c_result();

    double load[3] = {0};
    getloadavg(load, 3);

    struct rlimit rlimit;
    getrlimit(RLIMIT_NOFILE, &rlimit);

    struct rusage rusage;
    getrusage(RUSAGE_SELF, &rusage);

    addFormatStringBlock(block, 4500,
                         "github.com/truekenny/etracker - open-source BitTorrent tracker (%s)\n\n"
                         "start_time = %.24s\n" "thread_number = %d\n\n"

                         "Load Avg = %.2f %.2f %.2f\n"
                         "Interval = %'d\n"
                         "Active sockets: %'d (rlimit %'llu/%'llu)\n\n"

                         "Request per second ~ %.2f\n\n"

                         "rusage.ru_maxrss = %'12ld\n\n"

                         "Malloc = %'12d\n"
                         "Calloc = %'12d\n"
                         "*alloc = %'12d\n"
                         "free   = %'12d\n"
                         "*alloc - free = %'12d\n\n"

                         "stats.http_200 = %'12d\n"
                         "stats.http_400 = %'12d\n"
                         "stats.http_403 = %'12d (Full Scrape)\n"
                         "stats.http_404 = %'12d\n"
                         "stats.http_405 = %'12d (Not GET)\n"
                         "stats.http_408 = %'12d (Timeout)\n"
                         "stats.http_413 = %'12d (Oversize)\n\n"

                         "stats.close_pass  = %'12d\n"
                         "stats.send_pass   = %'12d\n"
                         "stats.recv_pass   = %'12d\n"
                         "stats.accept_pass = %'12d\n\n"

                         "stats.close_failed  = %'12d\n"
                         "stats.send_failed   = %'12d\n"
                         "stats.recv_failed   = %'12d\n"
                         "stats.accept_failed = %'12d\n\n"

                         "stats.send_pass_udp = %'12d\n"
                         "stats.recv_pass_udp = %'12d\n\n"

                         "stats.send_failed_udp = %'12d\n"
                         "stats.recv_failed_udp = %'12d\n\n"

                         "stats.keep_alive    = %'12d\n"
                         "stats.no_keep_alive = %'12d\n\n"

                         "stats.sent_bytes = %'15llu\n"
                         "stats.recv_bytes = %'15llu\n\n"

                         "stats.sent_bytes_udp = %'15llu\n"
                         "stats.recv_bytes_udp = %'15llu\n\n"

                         "stats.announce = %'12d\n"
                         "stats.scrape   = %'12d\n\n"

                         "stats.connect_udp  = %'12d\n"
                         "stats.announce_udp = %'12d\n"
                         "stats.scrape_udp   = %'12d\n\n",
                         REVISION,
                         ctime(&stats->time), threadNumber,

                         load[0], load[1], load[2],
                         interval,
                         stats->accept_pass - stats->close_pass - stats->close_failed,
                         rlimit.rlim_cur, rlimit.rlim_max,

                         getRps(rps),

                         rusage.ru_maxrss,

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
