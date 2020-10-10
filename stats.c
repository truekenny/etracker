#include <stdlib.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>
#include "stats.h"
#include "alloc.h"
#include "interval.h"

#if !defined(REVISION)
#define REVISION "UNKNOWN"
#endif

void
formatStats(int threadNumber, struct block *block, struct stats *stats, struct interval *interval, struct rps *rps) {
    struct c_countChanges *countChanges = c_result();

    double load[3] = {0};
    getloadavg(load, 3);

    struct rlimit rlimit;
    getrlimit(RLIMIT_NOFILE, &rlimit);

    struct rusage rusage;
    getrusage(RUSAGE_SELF, &rusage);

    char *background = stats->failed ? "#fee" : "white";

    addFormatStringBlock(block, 4500,
                         "<!DOCTYPE html>\n"
                         "<html lang='en'>"
                         "<head>"
                         "<title>etracker stats</title>"
                         "<meta name='viewport' content='width=device-width, initial-scale=1'>"
                         "</head>"
                         "<body style='background: %s'>"
                         "<div style='white-space: pre-wrap; font-family: monospace'>"

                         "github.com/truekenny/etracker - open-source BitTorrent tracker (%s)\n\n"
                         "start_time = %.24s (%dd) (%df)\n"
                         "thread_number = %d\n\n"

                         "load_avg = %.2f %.2f %.2f\n"
                         "interval = %'d (%'d->%'d)\n"
                         "active_sockets = %'d (rlimit %'llu/%'llu)\n\n"

                         "requests_per_second = tcp: %.2f, udp: %.2f\n\n"

                         "rusage.ru_maxrss = %'12ld\n\n"

                         "malloc        = %'15llu\n"
                         "calloc        = %'15llu\n"
                         "*alloc        = %'15llu\n"
                         "free          = %'15llu\n"
                         "*alloc - free = %'15llu\n\n"

                         "stats.http_101 = %'12d\n"
                         "stats.http_200 = %'12d\n"
                         "stats.http_400 = %'12d\n"
                         "stats.http_401 = %'12d\n"
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

                         "stats.recv_failed_read_0         = %'12d\n"
                         "stats.recv_failed_read_sub_0     = %'12d\n"
                         "stats.recv_failed_read_not_equal = %'12d\n\n"

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
                         "stats.scrape_udp   = %'12d\n\n"

                         "</div>"
                         "</body>"
                         "</html>",
                         background,
                         REVISION,
                         ctime(&stats->time), (time(NULL) - stats->time) / 86400, stats->failed,
                         threadNumber,

                         load[0], load[1], load[2],
                         interval->interval, interval->previousInterval, interval->requireInterval,
                         stats->accept_pass - stats->close_pass - stats->close_failed,
                         rlimit.rlim_cur, rlimit.rlim_max,

                         getRps(rps, RPS_TCP),
                         getRps(rps, RPS_UDP),

                         rusage.ru_maxrss,

                         countChanges->countMalloc,
                         countChanges->countCalloc,
                         countChanges->countMalloc + countChanges->countCalloc,
                         countChanges->countFree,
                         countChanges->countMalloc + countChanges->countCalloc - countChanges->countFree,

                         stats->http_101,
                         stats->http_200,
                         stats->http_400,
                         stats->http_401,
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

                         stats->recv_failed_read_0,
                         stats->recv_failed_read_sub_0,
                         stats->recv_failed_read_not_equal,

                         stats->send_pass_udp, stats->recv_pass_udp,

                         stats->send_failed_udp, stats->recv_failed_udp,

                         stats->keep_alive, stats->no_keep_alive,

                         stats->sent_bytes, stats->recv_bytes,

                         stats->sent_bytes_udp, stats->recv_bytes_udp,

                         stats->announce, stats->scrape,

                         stats->connect_udp, stats->announce_udp, stats->scrape_udp
    );
}
