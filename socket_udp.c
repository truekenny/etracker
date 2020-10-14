#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <pthread.h>
#include <unistd.h>
#include <stdatomic.h>
#include <errno.h>
#include "socket_udp.h"
#include "alloc.h"
#include "thread_client_udp.h"
#include "socket_udp_structure.h"
#include "string.h"
#include "udp_request.h"
#include "thread.h"
#include "sem.h"
#include "exit_code.h"
#include "interval.h"
#include "geoip.h"

// Размер заголовка пакета scrape + 74 x info_hash (по протоколу это максимальное кол-во)
#define SOCKET_UDP_RECEIVED_UDP_MESSAGE_LENGTH 1496

void checkSize() {
    if (sizeof(struct connectRequest) != 16) {
        exitPrint(EXIT_CODE_SIZE_CONNECT_REQUEST, __FILE__, EXIT_CODE_PRINT_ERROR_NO);
    }

    if (sizeof(struct connectResponse) != 16) {
        exitPrint(EXIT_CODE_SIZE_CONNECT_RESPONSE, __FILE__, EXIT_CODE_PRINT_ERROR_NO);
    }

    if (sizeof(struct announceRequest) != 98) {
        exitPrint(EXIT_CODE_SIZE_ANNOUNCE_REQUEST, __FILE__, EXIT_CODE_PRINT_ERROR_NO);
    }

    if (sizeof(struct announceHeadResponse) != 20) {
        exitPrint(EXIT_CODE_SIZE_ANNOUNCE_HEAD_RESPONSE, __FILE__, EXIT_CODE_PRINT_ERROR_NO);
    }

    if (sizeof(struct announcePeerResponse) != 6) {
        exitPrint(EXIT_CODE_SIZE_ANNOUNCE_PEER_RESPONSE, __FILE__, EXIT_CODE_PRINT_ERROR_NO);
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
    struct interval *interval = args->interval;
    struct rps *rps = args->rps;
    long workers = args->workers;
    unsigned int *maxPeersPerResponse = args->maxPeersPerResponse;
    struct list *websockets = args->websockets;
    struct geoip *geoip = args->geoip;

    c_free(args);

    int serverSocket;
    char receivedMessage[SOCKET_UDP_RECEIVED_UDP_MESSAGE_LENGTH + 1];
    struct sockaddr_in serverAddr, clientAddr;

    // Creating socket file descriptor
    if ((serverSocket = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        exitPrint(EXIT_CODE_SOCKET_UDP_CREATE, __FILE__, EXIT_CODE_PRINT_ERROR_YES);
    }

    memset(&serverAddr, 0, sizeof(serverAddr));
    memset(&clientAddr, 0, sizeof(clientAddr));

    // Filling server information
    serverAddr.sin_family = AF_INET; // IPv4
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(serverPort);

    // Bind the socket with the server address
    if (bind(serverSocket, (const struct sockaddr *) &serverAddr, sizeof(serverAddr)) < 0) {
        exitPrint(EXIT_CODE_BIND_UDP, __FILE__, EXIT_CODE_PRINT_ERROR_YES);
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

        clientUdpArgs->websockets = websockets;
        clientUdpArgs->geoip = geoip;

        // Поток
        pthread_t udpClientThread;
        if (pthread_create(&udpClientThread, NULL, (void *(*)(void *)) clientUdpHandler, clientUdpArgs) != 0) {
            exitPrint(EXIT_CODE_UDP_CLIENT_THREAD, __FILE__, EXIT_CODE_PRINT_ERROR_YES);
        }
    }

    int sockAddrSize, receivedSize;

    puts("Waiting UDP for incoming packets...");
    sockAddrSize = sizeof(struct sockaddr_in);

    unsigned long receiveCount = 0;

    while (1) {
        receivedSize = recvfrom(serverSocket, (char *) receivedMessage, SOCKET_UDP_RECEIVED_UDP_MESSAGE_LENGTH,
                                MSG_WAITALL, (struct sockaddr *) &clientAddr,
                                (socklen_t *) &sockAddrSize);

        if (receivedSize == 0) {
            printf("Recvfrom return 0\n");
            stats->recv_failed_udp++;

            continue;
        }

        if (receivedSize < 0) {
            incErrno(stats->recv_errno_udp);
            printf("Recvfrom return < 0\n");
            stats->recv_failed_udp++;

            continue;
        }

        updateRps(rps, RPS_PROTOCOL_UDP);
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

    exitPrint(EXIT_CODE_SOCKET_UDP_END, __FILE__, EXIT_CODE_PRINT_ERROR_NO);

    return NULL;
}
