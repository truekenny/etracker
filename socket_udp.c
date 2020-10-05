#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <pthread.h>
#include <unistd.h>
#include <stdatomic.h>
#include "socket_udp.h"
#include "alloc.h"
#include "thread_client_udp.h"
#include "socket_udp_structure.h"
#include "string.h"
#include "udp_request.h"
#include "thread.h"
#include "sem.h"

// Размер заголовка пакета scrape + 74 x info_hash (по протоколу это максимальное кол-во)
#define RECEIVED_UDP_MESSAGE_LENGTH 1496
#define MSG_CONFIRM_ 0

void checkSize() {
    if (sizeof(struct connectRequest) != 16) {
        printf("Failed packed: sizeof(struct connectRequest) != 16\n");
        exit(204);
    }

    if (sizeof(struct connectResponse) != 16) {
        printf("Failed packed: sizeof(struct connectResponse) != 16\n");
        exit(205);
    }

    if (sizeof(struct announceRequest) != 98) {
        printf("Failed packed: sizeof(struct announceRequest) != 98\n");
        exit(206);
    }

    if (sizeof(struct announceHeadResponse) != 20) {
        printf("Failed packed: sizeof(struct announceHeadResponse) != 20\n");
        exit(207);
    }

    if (sizeof(struct announcePeerResponse) != 6) {
        printf("Failed packed: sizeof(struct announcePeerResponse) != 6\n");
        exit(208);
    }
}

void *serverUdpHandler(struct serverUdpArgs *args) {
    pthreadSetName(pthread_self(), "UDP listen");

    checkSize();
    struct udpRequest
            *firstRequest = {0},
            *lastRequest = {0};

    pthread_cond_t signalRequest = PTHREAD_COND_INITIALIZER;
    pthread_mutex_t mutexSignalRequest = PTHREAD_MUTEX_INITIALIZER;
    struct rk_sema semaphoreRequest = {0};
    rk_sema_init(&semaphoreRequest, 1);

    struct stats *stats = args->stats;
    struct list *torrentList = args->torrentList;
    unsigned short serverPort = args->port;
    _Atomic(unsigned int) *interval = args->interval;
    struct rps *rps = args->rps;
    long workers = args->workers;
    unsigned int *maxPeersPerResponse = args->maxPeersPerResponse;
    c_free(args);

    int serverSocket;
    char receivedMessage[RECEIVED_UDP_MESSAGE_LENGTH + 1];
    struct sockaddr_in serverAddr, clientAddr;

    // Creating socket file descriptor
    if ((serverSocket = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Socket UDP creation failed");

        exit(201);
    }

    memset(&serverAddr, 0, sizeof(serverAddr));
    memset(&clientAddr, 0, sizeof(clientAddr));

    // Filling server information
    serverAddr.sin_family = AF_INET; // IPv4
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(serverPort);

    // Bind the socket with the server address
    if (bind(serverSocket, (const struct sockaddr *) &serverAddr, sizeof(serverAddr)) < 0) {
        perror("Bind UDP failed");

        exit(202);
    }

    // Starting Workers
    for (int threadNumber = 0; threadNumber < workers; threadNumber++) {
        printf("Starting UDP worker %d/%ld\n", threadNumber, workers - 1);

        struct clientUdpArgs *clientUdpArgs = c_calloc(1, sizeof(struct clientUdpArgs));
        clientUdpArgs->torrentList = torrentList;
        clientUdpArgs->stats = stats;
        clientUdpArgs->serverSocket = serverSocket;
        clientUdpArgs->interval = interval;

        clientUdpArgs->threadNumber = threadNumber;

        clientUdpArgs->signalRequest = &signalRequest;
        clientUdpArgs->mutexSignalRequest = &mutexSignalRequest;
        clientUdpArgs->firstRequest = &firstRequest;
        clientUdpArgs->lastRequest = &lastRequest;
        clientUdpArgs->semaphoreRequest = &semaphoreRequest;
        clientUdpArgs->rps = rps;
        clientUdpArgs->maxPeersPerResponse = maxPeersPerResponse;

        // Поток
        pthread_t udpClientThread;
        if (pthread_create(&udpClientThread, NULL, (void *(*)(void *)) clientUdpHandler, clientUdpArgs) != 0) {
            perror("Could not create UDP announce thread");

            exit(203);
        }
    }

    int sockAddrSize, receivedSize;

    puts("Waiting UDP for incoming packets...");
    sockAddrSize = sizeof(struct sockaddr_in);

    unsigned long receiveCount = 0;

    while (1) {
        receivedSize = recvfrom(serverSocket, (char *) receivedMessage, RECEIVED_UDP_MESSAGE_LENGTH,
                                MSG_WAITALL, (struct sockaddr *) &clientAddr,
                                (socklen_t *) &sockAddrSize);

        if (receivedSize == 0) {
            printf("Recvfrom return 0\n");
            stats->recv_failed_udp++;

            continue;
        }

        if (receivedSize < 0) {
            printf("Recvfrom return < 0\n");
            stats->recv_failed_udp++;

            continue;
        }

        updateRps(rps, RPS_UDP);
        stats->recv_pass_udp++;
        stats->recv_bytes_udp += receivedSize;

        receiveCount++;

        // printHex(receivedMessage, receivedSize);

        struct block *block = initBlock();
        addStringBlock(block, receivedMessage, receivedSize);

        rk_sema_wait(&semaphoreRequest);
        addUdpRequest(&firstRequest, &lastRequest, clientAddr, block, receiveCount);
        rk_sema_post(&semaphoreRequest);

        // SIGNAL
        pthread_mutex_lock(&mutexSignalRequest);
        pthread_cond_signal(&signalRequest);
        pthread_mutex_unlock(&mutexSignalRequest);

        // Чтобы нормально работала подсветка кода в IDE
        if (rand() % 2 == 3) break;
    } // while recv

    puts("UDP server socket finished");
    exit(6);

    return NULL;
}
