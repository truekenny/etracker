#include <stdlib.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
// #include <stdio.h>
#include "stats.h"
#include "alloc.h"
#include "interval.h"

#if !defined(REVISION)
#define REVISION "UNKNOWN"
#endif

void printErrorArray(struct block *block, atomic_ullong *statErrno, char *name);

// void printUpdateArray(struct block *block, atomic_uint *statUpdate, char *name);

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
    // printf(
                         "<!DOCTYPE html>\n"
                         "<html lang='en'>"
                         "<head>"
                         "<title>etracker stats</title>"
                         "<meta name='viewport' content='width=450, initial-scale=1'>"
                         "<style>"
                         "th {text-align: left} "
                         "tr:nth-child(odd) {background-color: #eee} "
                         ".two td:first-child + td {text-align: right} "
                         ".three td:first-child + td + td {text-align: right} "
                         "</style>"
                         "</head>"
                         "<body style='background: %s'>"
                         "<div style='white-space: pre-wrap; font-family: monospace'>"

                         "github.com/truekenny/etracker - open-source BitTorrent tracker (%s)\n\n"
                         "start_time = %.24s (%ldd) (%df)\n"
                         "thread_number = %d\n\n"

                         "load_avg = %.2f %.2f %.2f\n"
                         "interval = %'d (%'d->%'d)\n"
                         "active_sockets = %'llu (rlimit %'llu/%'llu)\n\n"

                         "RPS:      TCP  TCP_MAX      UDP  UDP_MAX\n"
                         "IPv4: %'7.0f  %'7u  %'7.0f  %'7u\n"
                         "IPv6: %'7.0f  %'7u  %'7.0f  %'7u\n\n"

                         "<table class='two'>"
                         
                         "<tr><td>rusage.ru_maxrss <td>%'ld"

                         "<tr><td colspan=2>&nbsp;"

                         "<tr><td>malloc <td>%'llu"
                         "<tr><td>calloc <td>%'llu"
                         "<tr><td>*alloc <td>%'llu"
                         "<tr><td>free <td>%'llu"
                         "<tr><td>*alloc - free <td>%'llu"

                         "<tr><td colspan=2>&nbsp;"

                         "<tr><td>stats.http_101 <td>%'llu"
                         "<tr><td>stats.http_200 <td>%'llu"
                         "<tr><td>stats.http_400 <td>%'llu"
                         "<tr><td>stats.http_401 <td>%'llu"
                         "<tr><td>stats.http_403 (Full Scrape)<td>%'llu"
                         "<tr><td>stats.http_404 <td>%'llu"
                         "<tr><td>stats.http_405 (Not GET)<td>%'llu"
                         "<tr><td>stats.http_408 (Timeout)<td>%'llu"
                         "<tr><td>stats.http_413 (Oversize)<td>%'llu"

                         "<tr><td colspan=2>&nbsp;"

                         "<tr><td>stats.close_pass <td>%'llu"
                         "<tr><td>stats.send_pass <td>%'llu"
                         "<tr><td>stats.recv_pass <td>%'llu"
                         "<tr><td>stats.accept_pass <td>%'llu"

                         "<tr><td colspan=2>&nbsp;"

                         "<tr><td>stats.close_failed <td>%'llu"
                         "<tr><td>stats.send_skips <td>%'llu"
                         "<tr><td>stats.send_failed <td>%'llu"
                         "<tr><td>stats.recv_failed <td>%'llu"
                         "<tr><td>stats.accept_failed <td>%'llu"

                         "<tr><td colspan=2>&nbsp;"

                         "<tr><td>stats.recv_failed_read_0 <td>%'llu"
                         "<tr><td>stats.recv_failed_read_sub_0 <td>%'llu"
                         "<tr><td>stats.recv_failed_read_not_equal <td>%'llu"

                         "<tr><td colspan=2>&nbsp;"

                         "<tr><td>stats.send_pass_udp <td>%'llu"
                         "<tr><td>stats.recv_pass_udp <td>%'llu"

                         "<tr><td colspan=2>&nbsp;"

                         "<tr><td>stats.send_failed_udp <td>%'llu"
                         "<tr><td>stats.recv_failed_udp <td>%'llu"

                         "<tr><td colspan=2>&nbsp;"

                         "<tr><td>stats.keep_alive <td>%'llu"
                         "<tr><td>stats.no_keep_alive <td>%'llu"

                         "<tr><td colspan=2>&nbsp;"

                         "<tr><td>stats.sent_bytes <td>%'llu"
                         "<tr><td>stats.recv_bytes <td>%'llu"

                         "<tr><td colspan=2>&nbsp;"

                         "<tr><td>stats.sent_bytes_udp <td>%'llu"
                         "<tr><td>stats.recv_bytes_udp <td>%'llu"

                         "<tr><td colspan=2>&nbsp;"

                         "<tr><td>stats.announce <td>%'llu"
                         "<tr><td>stats.scrape <td>%'llu"

                         "<tr><td colspan=2>&nbsp;"

                         "<tr><td>stats.connect_udp <td>%'llu"
                         "<tr><td>stats.announce_udp <td>%'llu"
                         "<tr><td>stats.scrape_udp <td>%'llu"

                         "<tr><td colspan=2>&nbsp;"

                         "</table>",
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
                         stats->send_skips,
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

    addFormatStringBlock(block, 1000, "<table class='two three'>");

    printErrorArray(block, stats->close_errno, "close_errno");
    printErrorArray(block, stats->send_errno, "send_errno");
    printErrorArray(block, stats->recv_errno, "recv_errno");
    printErrorArray(block, stats->accept_errno, "accept_errno");

    printErrorArray(block, stats->send_errno_udp, "send_errno_udp");
    printErrorArray(block, stats->recv_errno_udp, "recv_errno_udp");

    addFormatStringBlock(block, 1000, "</table>");

    // printUpdateArray(block, stats->update_peer, "update_peer");

    addFormatStringBlock(block, 1000, "</div>" "</body>" "</html>");
}

void incErrno(atomic_ullong *statErrno) {
    int err_no = errno;

    if (err_no > STATS_ERRNO_MAX_INDEX)
        err_no = STATS_ERRNO_MAX_INDEX;

    statErrno[err_no]++;
}

void printErrorArray(struct block *block, atomic_ullong *statErrno, char *name) {
    addFormatStringBlock(block, 1000, "<tr><th colspan=3>%s "
                                      "<tr><th>errno <th>count <th>name", name);

    for (int err_no = 0; err_no <= STATS_ERRNO_MAX_INDEX; ++err_no) {
        if (statErrno[err_no] != 0) {
            addFormatStringBlock(block, 1000,
            // printf(
                                 "<tr><td>%'d <td>%'llu <td>'%s'\n",
                                 err_no, statErrno[err_no], strerror(err_no));
        }
    }
}

/*
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
*/

void updatePeerStat(struct stats *stats, unsigned int delay) {
    if (delay >= STATS_MAX_DELAY_BETWEEN_UPDATE_S)
        delay = 0;

    stats->update_peer[delay]++;
}
