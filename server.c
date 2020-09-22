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
#include "queue.h"
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

#if !defined(REVISION)
#define REVISION "UNKNOWN"
#endif

#define DEBUG 0
#define DEFAULT_PORT 3000
#define DEFAULT_MAX_PEER_PER_RESULT 60
#define DEFAULT_SOCKET_TIMEOUT 3

void setNobody();

int main(int argc, char *argv[]) {
    printf("Revision: %s\n", REVISION);

    setlocale(LC_NUMERIC, "");

    unsigned short port = (argc < 2) ? DEFAULT_PORT : atoi(argv[1]);
    unsigned int interval = (argc < 3) ? MAX_INTERVAL : atoi(argv[2]);
    long workers = (argc < 4) ? sysconf(_SC_NPROCESSORS_ONLN) : atoi(argv[3]);
    unsigned int maxPeersPerResponse = (argc < 5) ? DEFAULT_MAX_PEER_PER_RESULT : atoi(argv[4]);
    unsigned short socketTimeout = (argc < 6) ? DEFAULT_SOCKET_TIMEOUT : atoi(argv[5]);

    if (!port || !interval || !workers || !maxPeersPerResponse || !socketTimeout) {
        printf("./etracker [port] [interval] [workers] [maxPeersPerResponse] [socketTimeout]\n");

        exit(1);
    }

    printf("Starting configuration: port = %d, interval = %d, workers = %ld, maxPeersPerResponse = %u, socketTimeout = %u\n",
           port, interval, workers, maxPeersPerResponse, socketTimeout);

    printf("This system has %ld processors available.\n", sysconf(_SC_NPROCESSORS_ONLN));

    c_initSem();

    // vars
    struct list *queueList = initList(NULL, 0, STARTING_NEST, sizeof(int), ENABLE_SEMAPHORE, LITTLE_ENDIAN);
    struct list *socketList = initList(NULL, 1, STARTING_NEST, sizeof(int), ENABLE_SEMAPHORE, LITTLE_ENDIAN);
    struct list *torrentList = initList(NULL, 2, STARTING_NEST, PARAM_VALUE_LENGTH, ENABLE_SEMAPHORE, LITTLE_ENDIAN);

    struct stats *stats = c_calloc(1, sizeof(struct stats));
    stats->time = time(NULL);

    struct rps rps = {};


    if (RANDOM_DATA_INFO_HASH)
        printf("- Random data info_hash: %d\n", RANDOM_DATA_INFO_HASH);
    if (RANDOM_DATA_PEER_ID)
        printf("- Random data peer_id: %d\n", RANDOM_DATA_PEER_ID);

    // Start TCP
    pthread_t tcpServerThread;
    struct serverTcpArgs *serverTcpArgs = (struct serverTcpArgs *) c_malloc(sizeof(struct serverTcpArgs));
    serverTcpArgs->queueList = queueList;
    serverTcpArgs->torrentList = torrentList;
    serverTcpArgs->stats = stats;
    serverTcpArgs->port = port;
    serverTcpArgs->socketList = socketList;
    serverTcpArgs->interval = &interval;
    serverTcpArgs->rps = &rps;
    serverTcpArgs->workers = workers;
    serverTcpArgs->maxPeersPerResponse = &maxPeersPerResponse;
    serverTcpArgs->socketTimeout = &socketTimeout;

    if (pthread_create(&tcpServerThread, NULL, serverTcpHandler, (void *) serverTcpArgs) != 0) {
        perror("Could not create thread");

        return 101;
    }
    // Start UDP
    pthread_t udpServerThread;
    struct serverUdpArgs *serverUdpArgs = (struct serverUdpArgs *) c_malloc(sizeof(struct serverUdpArgs));
    serverUdpArgs->torrentList = torrentList;
    serverUdpArgs->stats = stats;
    serverUdpArgs->port = port;
    serverUdpArgs->interval = &interval;
    serverUdpArgs->rps = &rps;
    serverUdpArgs->workers = workers;
    serverUdpArgs->maxPeersPerResponse = &maxPeersPerResponse;

    if (pthread_create(&udpServerThread, NULL, serverUdpHandler, (void *) serverUdpArgs) != 0) {
        perror("Could not create thread");

        return 102;
    }

    // Надо успеть забиндить порт, а затем уже сбрасывать права
    sleep(1);
    setNobody();

    run15MinutesThread(torrentList, &interval, &rps);
    runGarbageSocketTimeoutThread(socketList, stats, &socketTimeout);

    printf("Join TCP Thread\n");
    pthread_join(tcpServerThread, NULL);
    printf("Join UDP Thread\n");
    pthread_join(udpServerThread, NULL);

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
