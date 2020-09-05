#include "stats.h"

void formatStats(int threadNumber, struct block *block, struct stats *stats) {
    addFormatStringBlock(block, 2500,
                         "Stats Offline:\n"
                         "start_time = %.24s\n" "thread_number = %d\n\n"

                         "stats.http_200 = %d\n" "stats.http_400 = %d\n"
                         "stats.http_403 = %d\n" "stats.http_404 = %d\n\n"

                         "stats.send_pass = %d\n" "stats.recv_pass = %d\n"
                         "stats.accept_pass = %d\n\n"

                         "stats.send_failed = %d\n"
                         "stats.recv_failed = %d\n"
                         "stats.recv_failed_after_success = %d\n"
                         "stats.accept_failed = %d\n\n"

                         "stats.send_pass_udp = %d\n" "stats.recv_pass_udp = %d\n\n"

                         "stats.send_failed_udp = %d\n" "stats.recv_failed_udp = %d\n\n"

                         "stats.keep_alive = %d\n" "stats.no_keep_alive = %d\n\n"

                         "stats.sent_bytes = %llu\n" "stats.recv_bytes = %llu\n\n"

                         "stats.sent_bytes_udp = %llu\n" "stats.recv_bytes_udp = %llu\n\n"
                         ,
                         ctime(&stats->time), threadNumber,

                         stats->http_200, stats->http_400,
                         stats->http_403, stats->http_404,

                         stats->send_pass, stats->recv_pass,
                         stats->accept_pass,

                         stats->send_failed,
                         stats->recv_failed,
                         stats->recv_failed_after_success,
                         stats->accept_failed,

                         stats->send_pass_udp, stats->recv_pass_udp,

                         stats->send_failed_udp, stats->recv_failed_udp,

                         stats->keep_alive, stats->no_keep_alive,

                         stats->sent_bytes, stats->recv_bytes,

                         stats->sent_bytes_udp, stats->recv_bytes_udp
    );
}
