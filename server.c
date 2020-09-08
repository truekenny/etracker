#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <pwd.h>
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

#define DEBUG 0

void setNobody();

int main(int argc, char *argv[]) {
    // Check program's arguments
    if (argc < 2) {
        printf("./server port\n");
        return 1;
    }

    printf("This system has %ld processors configured and "
           "%ld processors available.\n",
           sysconf(_SC_NPROCESSORS_CONF),
           sysconf(_SC_NPROCESSORS_ONLN));

    c_initSem();

    // Vars
    struct rk_sema semaphoreQueue = {0}; // Семафор
    struct queue *queue = NULL; // Очередь
    struct firstByteData firstByteData = {}; // Торренты и пиры
    struct stats *stats = c_calloc(1, sizeof(struct stats));
    stats->time = time(NULL);

    initSem(&firstByteData);

    // Инициализация семафора
    rk_sema_init(&semaphoreQueue, 1);

    runGarbageCollectorThread(&firstByteData);

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
    serverTcpArgs->port = argv[1];
    if (pthread_create(&tcpServerThread, NULL, serverTcpHandler, (void *) serverTcpArgs) != 0) {
        perror("Could not create thread");

        return 101;
    }
    // Start TCP
    pthread_t udpServerThread;
    struct serverUdpArgs *serverUdpArgs = (struct serverUdpArgs *) c_malloc(sizeof(struct serverUdpArgs));
    serverUdpArgs->firstByteData = &firstByteData;
    serverUdpArgs->stats = stats;
    serverUdpArgs->port = argv[1];
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
