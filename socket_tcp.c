#include <stddef.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <assert.h>
#include <memory.h>
#include "socket_tcp.h"
#include "thread_client_tcp.h"
#include "socket.h"
#include "alloc.h"
#include "equeue.h"

#define DEBUG 0
#define DEBUG_KQUEUE 0
#define SOCKET_QUEUE_LENGTH 150
#define EVENTS_EACH_LOOP 32

void *serverTcpHandler(void *args) {
    struct stats *stats = ((struct serverTcpArgs *) args)->stats;
    struct rk_sema *semaphoreQueue = ((struct serverTcpArgs *) args)->semaphoreQueue;
    struct queue **queue = ((struct serverTcpArgs *) args)->queue;
    struct firstByteData *firstByteData = ((struct serverTcpArgs *) args)->firstByteData;
    char *port = ((struct serverTcpArgs *) args)->port;
    c_free(args);

    long coreCount = sysconf(_SC_NPROCESSORS_ONLN);

    int serverSocket;
    struct sockaddr_in serverAddr;

    // Create socket
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1) {
        perror("Socket TCP creation failed");

        exit(2);
    }
    DEBUG && puts("Socket created");

    //Prepare the sockaddr_in structure
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(atoi(port));

    // Reuse
    int option = 1;
    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option)) < 0) {
        perror("SO_REUSEADDR failed");

        exit(3);
    }

    // Bind
    if (bind(serverSocket, (struct sockaddr *) &serverAddr, sizeof(serverAddr)) < 0) {
        perror("Bind TCP failed");

        exit(4);
    }
    DEBUG && puts("Bind done");

    int *equeue = c_calloc(coreCount, sizeof(int));

    pthread_t tcpClientThread;
    // Кол-во воркеров = кол-ву ядер
    for (int threadNumber = 0; threadNumber < coreCount; threadNumber++) {
        struct clientTcpArgs *clientTcpArgs = (struct clientTcpArgs *) c_malloc(sizeof(struct clientTcpArgs));
        clientTcpArgs->threadNumber = threadNumber;
        clientTcpArgs->semaphoreQueue = semaphoreQueue;
        clientTcpArgs->queue = queue;
        clientTcpArgs->firstByteData = firstByteData;
        clientTcpArgs->stats = stats;

        clientTcpArgs->equeue = equeue[threadNumber] = initEqueue();

        if (pthread_create(&tcpClientThread, NULL, clientTcpHandler, (void *) clientTcpArgs) != 0) {
            perror("Could not create TCP thread");

            exit(5);
        }    // end of workers
    }

    // Listen
    listen(serverSocket, SOCKET_QUEUE_LENGTH);

    puts("Waiting TCP for incoming connections...");


    int clientSocket;

    struct sockaddr_in clientAddr;
    socklen_t sockAddrSize = sizeof(struct sockaddr_in);
    unsigned long currentThread = 0;

    while ((clientSocket = accept(serverSocket, (struct sockaddr *) &clientAddr, &sockAddrSize))) {
        if (clientSocket == -1) {
            stats->accept_failed++;
            perror("Accept failed"); // Timeout

            continue;
        }
        stats->accept_pass++;

        //setTimeout(clientSocket);

        addClientEqueue(equeue[(currentThread++) % coreCount], clientSocket);
        DEBUG_KQUEUE && printf("socket_tcp.c: Got connection!\n");

        int flags = fcntl(clientSocket, F_GETFL, 0);
        if (flags < 0) {
            if (DEBUG)
                perror("Flags failed");
        } else
            fcntl(clientSocket, F_SETFL, flags | O_NONBLOCK);

        DEBUG && puts("Handler assigned");
    }

    puts("TCP server socket finished");
    exit(6);

    return NULL;
}
