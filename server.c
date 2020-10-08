#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <pwd.h>
#include <locale.h>
#include <sys/resource.h>
#include <errno.h>
#include "sem.h"
#include "alloc.h"
#include "socket.h"
#include "uri.h"
#include "socket_tcp.h"
#include "socket_udp.h"
#include "thread_garbage.h"
#include "socket_garbage.h"
#include "interval.h"
#include "rps.h"
#include "list.h"
#include "data_garbage.h"
#include "argument.h"

#if !defined(REVISION)
#define REVISION "UNKNOWN"
#endif

void setNobody();

void setLimit(long long soft, int limitName);

void setLocale(char *locale);

int main(int argc, char *argv[]) {
    printf("Revision: %s\n", REVISION);

    struct arguments *arguments = parseArguments(argc, argv);

    setLocale(arguments->locale);

    printf("Starting configuration:\n"
           "  port = %d\n"
           "  interval = %d\n"
           "  workers = %ld\n"
           "  maxPeersPerResponse = %u\n"
           "  socketTimeout = %u\n"
           "  keepAlive = %u\n"
           "  minInterval = %u\n"
           "  maxInterval = %u\n"
           "  noTcp = %u\n"
           "  noUdp = %u\n"
           "  charset = %s\n"
           "  locale = %s\n",
           arguments->port, arguments->interval, arguments->workers, arguments->maxPeersPerResponse,
           arguments->socketTimeout, arguments->keepAlive, arguments->minInterval, arguments->maxInterval,
           arguments->noTcp, arguments->noUdp, arguments->charset, arguments->locale);

    printf("This system has %ld processors available.\n", sysconf(_SC_NPROCESSORS_ONLN));

    c_initSem(); // Семафор для точного подсчёта alloc блоков
    setLimit(arguments->nofile, RLIMIT_NOFILE);
    setLimit(arguments->core, RLIMIT_CORE);

    // vars
    struct list **socketLists = c_calloc(arguments->workers, sizeof(void *));

    for (int threadNumber = 0; threadNumber < arguments->workers; threadNumber++) {
        socketLists[threadNumber] =
                initList(NULL, 1, STARTING_NEST, sizeof(int),
                         ENABLE_SEMAPHORE_LEAF | ENABLE_SEMAPHORE_GLOBAL, LITTLE_ENDIAN);
    }
    struct list *torrentList = initList(NULL, 2, STARTING_NEST, PARAM_VALUE_LENGTH,
                                        ENABLE_SEMAPHORE_LEAF, LITTLE_ENDIAN);

    struct stats *stats = c_calloc(1, sizeof(struct stats));
    stats->time = time(NULL);
    stats->failed = arguments->failed;

    struct rps rps = {};


    if (RANDOM_DATA_INFO_HASH)
        printf("- Random data info_hash: %d\n", RANDOM_DATA_INFO_HASH);
    if (RANDOM_DATA_PEER_ID)
        printf("- Random data peer_id: %d\n", RANDOM_DATA_PEER_ID);

    // Start TCP
    pthread_t tcpServerThread;
    if (!arguments->noTcp) {
        struct serverTcpArgs *serverTcpArgs = (struct serverTcpArgs *) c_malloc(sizeof(struct serverTcpArgs));
        serverTcpArgs->torrentList = torrentList;
        serverTcpArgs->stats = stats;
        serverTcpArgs->port = arguments->port;
        serverTcpArgs->socketLists = socketLists;
        serverTcpArgs->interval = &arguments->interval;
        serverTcpArgs->rps = &rps;
        serverTcpArgs->workers = arguments->workers;
        serverTcpArgs->maxPeersPerResponse = &arguments->maxPeersPerResponse;
        serverTcpArgs->socketTimeout = &arguments->socketTimeout;
        serverTcpArgs->keepAlive = &arguments->keepAlive;
        serverTcpArgs->charset = arguments->charset;

        if (pthread_create(&tcpServerThread, NULL, (void *(*)(void *)) serverTcpHandler, serverTcpArgs) != 0) {
            perror("Could not create thread");

            return 101;
        }
    }
    // Start UDP
    pthread_t udpServerThread;
    if (!arguments->noUdp) {
        struct serverUdpArgs *serverUdpArgs = (struct serverUdpArgs *) c_malloc(sizeof(struct serverUdpArgs));
        serverUdpArgs->torrentList = torrentList;
        serverUdpArgs->stats = stats;
        serverUdpArgs->port = arguments->port;
        serverUdpArgs->interval = &arguments->interval;
        serverUdpArgs->rps = &rps;
        serverUdpArgs->workers = arguments->workers;
        serverUdpArgs->maxPeersPerResponse = &arguments->maxPeersPerResponse;

        if (pthread_create(&udpServerThread, NULL, (void *(*)(void *)) serverUdpHandler, serverUdpArgs) != 0) {
            perror("Could not create thread");

            return 102;
        }
    }

    // Надо успеть забиндить порт, а затем уже сбрасывать права
    sleep(1);
    setNobody();

    run15MinutesThread(torrentList, &arguments->interval, &rps, arguments->minInterval, arguments->maxInterval);
    runGarbageSocketTimeoutThread(socketLists, stats, &arguments->socketTimeout, arguments->workers);

    if (!arguments->noTcp) {
        printf("Join TCP Thread\n");
        pthread_join(tcpServerThread, NULL);
    }

    if (!arguments->noUdp) {
        printf("Join UDP Thread\n");
        pthread_join(udpServerThread, NULL);
    }

    printf("Bye-Bye\n");

    return EXIT_SUCCESS;
}

void setNobody() {
    const char *name = "nobody";
    struct passwd *p;
    if ((p = getpwnam(name)) == NULL) {
        perror("Getpwnam failed");
    } else {
        if (setgid(p->pw_gid)) {
            perror("Setgid failed");
        }
        if (setuid(p->pw_uid)) {
            perror("Setuid failed");
        }
    };
}

void setLimit(long long soft, int limitName) {
    if (!soft)
        return;

    struct rlimit rlimit;

    if (getrlimit(limitName, &rlimit) == 0) {
        printf("Current %d -> soft=%llu, hard=%llu\n", limitName,
               (unsigned long long) rlimit.rlim_cur, (unsigned long long) rlimit.rlim_max);

        if (soft == -1)
            rlimit.rlim_cur = rlimit.rlim_max;
        else
            rlimit.rlim_cur = soft;

        if (setrlimit(limitName, &rlimit) != -1)
            printf("New %d -> soft=%'llu, hard=%'llu\n", limitName,
                   (unsigned long long) rlimit.rlim_cur, (unsigned long long) rlimit.rlim_max);
        else
            printf("setrlimit error: %d: %s (maybe %'llu too large)\n", errno, strerror(errno), soft);
    } else
        printf("getrlimit error: %d: %s\n", errno, strerror(errno));
}

void setLocale(char *locale) {
    char *_locale = "";

    if (locale != NULL)
        _locale = locale;

    if (setlocale(LC_NUMERIC, _locale) == NULL)
        printf("setlocale failed: %d: %s\n", errno, strerror(errno));
}
