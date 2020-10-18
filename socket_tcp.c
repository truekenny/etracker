#include <stddef.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <assert.h>
#include <memory.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdatomic.h>
#include <errno.h>
#include "socket_tcp.h"
#include "thread_client_tcp.h"
#include "socket.h"
#include "alloc.h"
#include "equeue.h"
#include "socket_garbage.h"
#include "thread.h"
#include "uri.h"
#include "exit_code.h"
#include "interval.h"

#define SOCKET_TCP_SOCKET_QUEUE_LENGTH 150

#define SOCKET_TCP_MAX_CWD 500
#define SOCKET_TCP_WEB_PATH "web"
#define SOCKET_TCP_SEPARATOR_PATH "/"


void *serverTcpHandler(struct serverTcpArgs *args) {
    pthreadSetName(pthread_self(), "TCP listen");

    struct stats *stats = args->stats;
    struct list *torrentList = args->torrentList;
    unsigned short port = args->port;
    struct list **socketLists = args->socketLists;
    struct interval *interval = args->interval;
    struct rps *rps = args->rps;
    long workers = args->workers;
    unsigned int *maxPeersPerResponse = args->maxPeersPerResponse;
    unsigned short *socketTimeout = args->socketTimeout;
    unsigned char *keepAlive = args->keepAlive;
    char *charset = args->charset;
    struct list *websockets = args->websockets;
    struct geoip *geoip = args->geoip;
    char *xForwardedFor = args->xForwardedFor;

    c_free(args);

    if (chdir(SOCKET_TCP_WEB_PATH) == -1) {
        exitPrint(EXIT_CODE_CHDIR, __FILE__, EXIT_CODE_PRINT_ERROR_YES);
    }

    char webRoot[SOCKET_TCP_MAX_CWD + 1];

    if (getcwd(webRoot, SOCKET_TCP_MAX_CWD) == NULL) {
        exitPrint(EXIT_CODE_CWD, __FILE__, EXIT_CODE_PRINT_ERROR_YES);
    }
    strcat(webRoot, SOCKET_TCP_SEPARATOR_PATH);
    printf("webRoot: '%s'\n", webRoot);

    struct block *authorizationHeader = initBlock();

    int serverSocket;
    struct sockaddr_in6 serverAddr = {};

    // Create socket
    serverSocket = socket(AF_INET6, SOCK_STREAM, 0);
    if (serverSocket == -1) {
        exitPrint(EXIT_CODE_SOCKET_TCP_CREATE, __FILE__, EXIT_CODE_PRINT_ERROR_YES);
    }

    //Prepare the sockaddr_in6 structure
    serverAddr.sin6_family = AF_INET6;
    serverAddr.sin6_port = htons(port);

    // Reuse
    int option = 1;
    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option)) < 0) {
        exitPrint(EXIT_CODE_REUSEADDR, __FILE__, EXIT_CODE_PRINT_ERROR_YES);
    }

    // Bind
    if (bind(serverSocket, (struct sockaddr *) &serverAddr, sizeof(serverAddr)) < 0) {
        exitPrint(EXIT_CODE_BIND_TCP, __FILE__, EXIT_CODE_PRINT_ERROR_YES);
    }

    int *equeue = c_calloc(workers, sizeof(int));

    pthread_t tcpClientThread;
    // Кол-во воркеров = кол-ву ядер
    for (int threadNumber = 0; threadNumber < workers; threadNumber++) {
        printf("Starting TCP worker %d/%ld\n", threadNumber, workers - 1);

        struct clientTcpArgs *clientTcpArgs = (struct clientTcpArgs *) c_malloc(sizeof(struct clientTcpArgs));
        clientTcpArgs->threadNumber = threadNumber;
        clientTcpArgs->torrentList = torrentList;
        clientTcpArgs->stats = stats;

        clientTcpArgs->equeue = equeue[threadNumber] = initEqueue();

        clientTcpArgs->socketList = socketLists[threadNumber];

        clientTcpArgs->interval = interval;
        clientTcpArgs->rps = rps;

        clientTcpArgs->authorizationHeader = authorizationHeader;
        clientTcpArgs->maxPeersPerResponse = maxPeersPerResponse;
        clientTcpArgs->socketTimeout = socketTimeout;
        clientTcpArgs->keepAlive = keepAlive;
        clientTcpArgs->charset = charset;
        clientTcpArgs->webRoot = webRoot;
        clientTcpArgs->websockets = websockets;
        clientTcpArgs->geoip = geoip;

        clientTcpArgs->xForwardedFor = xForwardedFor;

        if (pthread_create(&tcpClientThread, NULL, (void *(*)(void *)) clientTcpHandler, clientTcpArgs) != 0) {
            exitPrint(EXIT_CODE_TCP_CLIENT_THREAD, __FILE__, EXIT_CODE_PRINT_ERROR_YES);
        }    // end of workers
    }

    // Listen
    listen(serverSocket, SOCKET_TCP_SOCKET_QUEUE_LENGTH);

    puts("Waiting TCP for incoming connections...");


    int clientSocket;

    struct sockaddr_in6 clientAddr;
    socklen_t sockAddrSize = sizeof(struct sockaddr_in6);
    long currentThread = 0;

    while (1) {
        clientSocket = accept(serverSocket, (struct sockaddr *) &clientAddr, &sockAddrSize);

        if (clientSocket == 0) {
            printf("Accept return 0\n");
            stats->accept_failed++;

            continue;
        }

        if (clientSocket < 0) {
            incErrno(stats->accept_errno);

            stats->accept_failed++;

            continue;
        }

        stats->accept_pass++;

        currentThread++;
        if (currentThread == workers)
            currentThread = 0;

        int equeueThread = equeue[currentThread];

        updateSocketL(socketLists[currentThread], clientSocket, equeueThread, 1);

        addClientEqueue(equeueThread, clientSocket);

        int flags = fcntl(clientSocket, F_GETFL, 0);
        if (flags >= 0)
            fcntl(clientSocket, F_SETFL, flags | O_NONBLOCK);

        // Чтобы нормально работала подсветка кода в IDE
        if (rand() % 2 == 3) break;
    }

    exitPrint(EXIT_CODE_SOCKET_TCP_END, __FILE__, EXIT_CODE_PRINT_ERROR_NO);

    return NULL;
}
