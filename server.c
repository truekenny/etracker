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

    c_initSem();

    // Vars
    struct rk_sema sem = {0}; // Семафор
    struct queue *first = NULL; // Очередь
    struct firstByte firstByte = {0}; // Торренты и пиры
    struct stats *stats = c_calloc(1, sizeof(struct stats));
    stats->time = time(NULL);

    initSem(&firstByte);

    // Инициализация семафора
    rk_sema_init(&sem, 1);

    runGarbageCollectorThread(&firstByte);

    DEBUG && printf("first = %p\n", first);
    DEBUG && printf("&first = %p\n", &first);

    if (INTERVAL < 100)
        printf("- Interval: %d\n", INTERVAL);
    if (RANDOM_DATA_INFO_HASH)
        printf("- Random data info_hash: %d\n", RANDOM_DATA_INFO_HASH);
    if (RANDOM_DATA_PEER_ID)
        printf("- Random data peer_id: %d\n", RANDOM_DATA_PEER_ID);

    // Start TCP
    pthread_t tcpServerThread;
    struct serverTcpArgs *serverTcpArgs = (struct serverTcpArgs *) c_malloc(sizeof(struct serverTcpArgs));
    serverTcpArgs->sem = &sem;
    serverTcpArgs->first = &first;
    serverTcpArgs->firstByte = &firstByte;
    serverTcpArgs->stats = stats;
    serverTcpArgs->port = argv[1];
    if (pthread_create(&tcpServerThread, NULL, serverTcpHandler, (void *) serverTcpArgs) != 0) {
        perror("Could not create thread");

        return 101;
    }
    // Start TCP
    pthread_t udpServerThread;
    struct serverUdpArgs *serverUdpArgs = (struct serverUdpArgs *) c_malloc(sizeof(struct serverUdpArgs));
    serverUdpArgs->firstByte = &firstByte;
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
