#include <stdlib.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include "stats.h"
#include "alloc.h"
#include "interval.h"

#if !defined(REVISION)
#define REVISION "UNKNOWN"
#endif

void printErrorArray(struct block *block, atomic_uint *statErrno, char *name);

void printUpdateArray(struct block *block, atomic_uint *statUpdate, char *name);

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
                         "start_time = %.24s (%ldd) (%df)\n"
                         "thread_number = %d\n\n"

                         "load_avg = %.2f %.2f %.2f\n"
                         "interval = %'d (%'d->%'d)\n"
                         "active_sockets = %'d (rlimit %'llu/%'llu)\n\n"

                         "RPS:      TCP  TCP_MAX      UDP  UDP_MAX\n"
                         "IPv4: %'7.0f  %'7u  %'7.0f  %'7u\n"
                         "IPv6: %'7.0f  %'7u  %'7.0f  %'7u\n\n"

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
                         "stats.scrape_udp   = %'12d\n\n",
                         background,
                         REVISION,
                         ctime(&stats->time), (time(NULL) - stats->time) / 86400, stats->failed,
                         threadNumber,

                         load[0], load[1], load[2],
                         interval->interval, interval->previousInterval, interval->requireInterval,
                         stats->accept_pass - stats->close_pass - stats->close_failed,
                         rlimit.rlim_cur, rlimit.rlim_max,

                         getRps(rps, RPS_PROTOCOL_TCP, RPS_VERSION_IPV4),
                         rps->max[RPS_PROTOCOL_TCP + RPS_VERSION_IPV4],
                         getRps(rps, RPS_PROTOCOL_UDP, RPS_VERSION_IPV4),
                         rps->max[RPS_PROTOCOL_UDP + RPS_VERSION_IPV4],
                         getRps(rps, RPS_PROTOCOL_TCP, RPS_VERSION_IPV6),
                         rps->max[RPS_PROTOCOL_TCP + RPS_VERSION_IPV6],
                         getRps(rps, RPS_PROTOCOL_UDP, RPS_VERSION_IPV6),
                         rps->max[RPS_PROTOCOL_UDP + RPS_VERSION_IPV6],

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

    printErrorArray(block, stats->close_errno, "close_errno");
    printErrorArray(block, stats->send_errno, "send_errno");
    printErrorArray(block, stats->recv_errno, "recv_errno");
    printErrorArray(block, stats->accept_errno, "accept_errno");

    printErrorArray(block, stats->send_errno_udp, "send_errno_udp");
    printErrorArray(block, stats->recv_errno_udp, "recv_errno_udp");

    printUpdateArray(block, stats->update_peer, "update_peer");

    addFormatStringBlock(block, 1000, "</div>" "</body>" "</html>");
}

void incErrno(atomic_uint *statErrno) {
    int err_no = errno;

    if (err_no > STATS_ERRNO_MAX_INDEX)
        err_no = STATS_ERRNO_MAX_INDEX;

    statErrno[err_no]++;
}

void printErrorArray(struct block *block, atomic_uint *statErrno, char *name) {
    addFormatStringBlock(block, 1000, "%s:\n", name);

    for (int err_no = 0; err_no <= STATS_ERRNO_MAX_INDEX; ++err_no) {
        if (statErrno[err_no] != 0) {
            addFormatStringBlock(block, 1000, "  errno = %'3d count = %'9u name = '%s'\n",
                                 err_no, statErrno[err_no], strerror(err_no));
        }
    }
}

void printUpdateArray(struct block *block, atomic_uint *statUpdate, char *name) {
    addFormatStringBlock(block, 1000, "%s:\n", name);

    unsigned int sum = 0;
    for (int delay = 0; delay < STATS_MAX_DELAY_BETWEEN_UPDATE_S; ++delay) {
        sum += statUpdate[delay];
    }

    if (!sum) sum = 1;  // Не хочу делить на 0

    for (int delay = 0; delay < STATS_MAX_DELAY_BETWEEN_UPDATE_S; ++delay) {
        if (statUpdate[delay] != 0) {
            addFormatStringBlock(block, 1000, "  delay = %'2d:%'02d count = %'9u / %5.2f%%\n",
                                 delay / 60,
                                 delay - (delay / 60) * 60,
                                 statUpdate[delay],
                                 (float) statUpdate[delay] * 100 / (float) sum);
        }
    }
}

void updatePeerStat(struct stats *stats, unsigned int delay) {
    if (delay >= STATS_MAX_DELAY_BETWEEN_UPDATE_S)
        delay = 0;

    stats->update_peer[delay]++;
}
