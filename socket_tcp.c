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

void *serverTcp(void *args) {
    struct stats *stats = ((struct server_tcp *) args)->stats;
    struct rk_sema *sem = ((struct server_tcp *) args)->sem;
    struct queue **first = ((struct server_tcp *) args)->first;
    struct firstByte *firstByte = ((struct server_tcp *) args)->firstByte;

    int serverSocket, clientSocket, sockAddrSize; // Сокеты и размер структуры sockaddr_in
    struct sockaddr_in serverAddr, clientAddr;
    char ip[16]; // IP адрес 123.123.123.123\0
    uint16_t port; // Порт клиентской стороны
    int threadCounter = 0; // Счётчик подключений


    // Create socket
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1) {
        perror("Could not create socket");

        exit(2);
    }
    DEBUG && puts("Socket created");

    //Prepare the sockaddr_in structure
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(atoi(((struct server_tcp *) args)->port));

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
        perror("Bind failed");

        exit(4);
    }
    DEBUG && puts("Bind done");

    // Listen
    listen(serverSocket, SOCKET_QUEUE_LENGTH);

    // Accept and incoming connection
    puts("Waiting for incoming connections...");
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

        pthread_t sniffer_thread;

        struct args *threadArguments = (struct args *) c_malloc(sizeof(struct args));
        threadArguments->sock = clientSocket;
        threadArguments->number = ++threadCounter;
        threadArguments->sem = sem;
        threadArguments->first = first;
        threadArguments->ip = clientAddr.sin_addr.s_addr;
        threadArguments->firstByte = firstByte;
        threadArguments->stats = stats;

        DEBUG && printf("Connection accepted: %s:%d sock:%d number:%d\n", ip, port, clientSocket, threadCounter);

        if (pthread_create(&sniffer_thread, NULL, connection_handler, (void *) threadArguments) != 0) {
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
