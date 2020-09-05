#include <stddef.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "socket_tcp.h"
#include "thread.h"
#include "socket.h"
#include "alloc.h"

#define DEBUG 0
#define SOCKET_QUEUE_LENGTH 150

void *serverTcpHandler(void *args) {
    struct stats *stats = ((struct serverTcpArgs *) args)->stats;
    struct rk_sema *sem = ((struct serverTcpArgs *) args)->sem;
    struct queue **first = ((struct serverTcpArgs *) args)->first;
    struct firstByte *firstByte = ((struct serverTcpArgs *) args)->firstByte;
    char *serverPort = ((struct serverTcpArgs *) args)->port;
    c_free(args);

    int serverSocket, clientSocket, sockAddrSize; // Сокеты и размер структуры sockaddr_in
    struct sockaddr_in serverAddr, clientAddr;
    char ip[16]; // IP адрес 123.123.123.123\0
    uint16_t port; // Порт клиентской стороны
    int threadCounter = 0; // Счётчик подключений


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
    serverAddr.sin_port = htons(atoi(serverPort));

    // Reuse
    int option = 1;
    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option)) < 0) {
        perror("SO_REUSEADDR failed");

        exit(3);
    }

    // Timeout
    // ? Возможно здесь это не надо
    // setTimeout(socket_desc);

    // Bind
    if (bind(serverSocket, (struct sockaddr *) &serverAddr, sizeof(serverAddr)) < 0) {
        perror("Bind TCP failed");

        exit(4);
    }
    DEBUG && puts("Bind done");

    // Listen
    listen(serverSocket, SOCKET_QUEUE_LENGTH);

    // Accept and incoming connection
    puts("Waiting TCP for incoming connections...");
    sockAddrSize = sizeof(struct sockaddr_in);

    while ((clientSocket = accept(serverSocket, (struct sockaddr *) &clientAddr, (socklen_t *) &sockAddrSize))) {
        if (clientSocket == -1) {
            stats->accept_failed++;
            perror("Accept failed"); // Timeout

            continue;
        }
        stats->accept_pass++;

        // fcntl(clientSocket, F_SETFL, O_NONBLOCK);
        setTimeout(clientSocket);

        inet_ntop(AF_INET, &(clientAddr.sin_addr), ip, INET_ADDRSTRLEN);
        port = ntohs(clientAddr.sin_port);

        pthread_t tcpClientThread;
        struct clientTcpArgs *clientTcpArgs = (struct clientTcpArgs *) c_malloc(sizeof(struct clientTcpArgs));
        clientTcpArgs->sock = clientSocket;
        clientTcpArgs->number = ++threadCounter;
        clientTcpArgs->sem = sem;
        clientTcpArgs->first = first;
        clientTcpArgs->ip = clientAddr.sin_addr.s_addr;
        clientTcpArgs->firstByte = firstByte;
        clientTcpArgs->stats = stats;

        DEBUG && printf("Connection accepted: %s:%d sock:%d number:%d\n", ip, port, clientSocket, threadCounter);

        if (pthread_create(&tcpClientThread, NULL, clientTcpHandler, (void *) clientTcpArgs) != 0) {
            perror("Could not create thread");

            exit(5);
        }

        DEBUG && puts("Handler assigned");
    }

    if (clientSocket < 0) {
        perror("Accept failed (end)");

        exit(6);
    }

    puts("TCP server socket finished");

    return NULL;
}
