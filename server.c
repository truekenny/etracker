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
#include "data_change.h"
#include "data_sem.h"
#include "socket_tcp.h"
#include "socket_udp.h"
#include "thread_garbage.h"
#include "socket_garbage.h"
#include "interval.h"
#include "rps.h"

#define DEBUG 0
#define DEFAULT_PORT 3000

void setNobody();

int main(int argc, char *argv[]) {
    setlocale(LC_NUMERIC, "");

    unsigned short port = (argc < 2) ? DEFAULT_PORT : atoi(argv[1]);
    unsigned int interval = (argc < 3) ? MAX_INTERVAL : atoi(argv[2]);

    if(!port || !interval) {
        printf("./server.o [port] [interval]\n");

        exit(1);
    }

    printf("Starting configuration: port = %d, interval = %d\n", port, interval);

    printf("This system has %ld processors configured and "
           "%ld processors available.\n",
           sysconf(_SC_NPROCESSORS_CONF),
           sysconf(_SC_NPROCESSORS_ONLN));

    c_initSem();

    // vars
    struct rk_sema semaphoreQueue = {0}; // Семафор для очереди
    struct queue *queue = NULL; // Очередь

    struct rk_sema semaphoreSocketPool = {0};
    struct socketPool *socketPool = NULL;

    struct firstByteData firstByteData = {}; // Торренты и пиры
    struct stats *stats = c_calloc(1, sizeof(struct stats));
    stats->time = time(NULL);

    struct rps rps = {};

    // init
    initSem(&firstByteData);

    // Инициализация семафоров
    rk_sema_init(&semaphoreQueue, 1);
    rk_sema_init(&semaphoreSocketPool, 1);

    run15MinutesThread(&firstByteData, &interval);
    runGarbageSocketPoolThread(&socketPool, &semaphoreSocketPool, stats);

    DEBUG && printf("first = %p\n", queue);
    DEBUG && printf("&first = %p\n", &queue);

    if (INTERVAL < 100)
        printf("- Interval: %d\n", INTERVAL);
    if (RANDOM_DATA_INFO_HASH)
        printf("- Random data info_hash: %d\n", RANDOM_DATA_INFO_HASH);
    if (RANDOM_DATA_PEER_ID)
        printf("- Random data peer_id: %d\n", RANDOM_DATA_PEER_ID);

    // Start TCP
    pthread_t tcpServerThread;
    struct serverTcpArgs *serverTcpArgs = (struct serverTcpArgs *) c_malloc(sizeof(struct serverTcpArgs));
    serverTcpArgs->semaphoreQueue = &semaphoreQueue;
    serverTcpArgs->queue = &queue;
    serverTcpArgs->firstByteData = &firstByteData;
    serverTcpArgs->stats = stats;
    serverTcpArgs->port = port;

    serverTcpArgs->semaphoreSocketPool = &semaphoreSocketPool;
    serverTcpArgs->socketPool = &socketPool;

    serverTcpArgs->interval = &interval;
    serverTcpArgs->rps = &rps;
    if (pthread_create(&tcpServerThread, NULL, serverTcpHandler, (void *) serverTcpArgs) != 0) {
        perror("Could not create thread");

        return 101;
    }
    // Start UDP
    pthread_t udpServerThread;
    struct serverUdpArgs *serverUdpArgs = (struct serverUdpArgs *) c_malloc(sizeof(struct serverUdpArgs));
    serverUdpArgs->firstByteData = &firstByteData;
    serverUdpArgs->stats = stats;
    serverUdpArgs->port = port;
    serverUdpArgs->interval = &interval;
    serverUdpArgs->rps = &rps;
    if (pthread_create(&udpServerThread, NULL, serverUdpHandler, (void *) serverUdpArgs) != 0) {
        perror("Could not create thread");

        return 102;
    }

    setNobody();

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
