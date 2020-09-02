#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <unistd.h>
#include "queue.h"
#include "sem.h"
#include "alloc.h"
#include "socket.h"
#include "uri.h"
#include "string.h"
#include "thread.h"
#include "data.h"

#define DEBUG 0
#define SOCKET_QUEUE_LENGTH 150

int main(int argc, char *argv[]) {
    // Check program's arguments
    if (argc < 2) {
        printf("./server port\n");
        return 1;
    }

    // Vars
    struct rk_sema sem; // Семафор
    int serverSocket, clientSocket, sockAddrSize; // Сокеты и размер структуры sockaddr_in
    struct sockaddr_in serverAddr, clientAddr;
    char ip[16]; // IP адрес 123.123.123.123\0
    uint16_t port; // Порт клиентской стороны
    int threadCounter = 0; // Счётчик подключений
    struct queue *first = NULL; // Очередь
    struct firstByte firstByte = {0}; // Торренты и пиры

    initSem(&firstByte);

    // Инициализация семафора
    rk_sema_init(&sem, 1);

    // Create socket
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1) {
        perror("Could not create socket");

        return 1;
    }
    DEBUG && puts("Socket created");

    //Prepare the sockaddr_in structure
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(atoi(argv[1]));

    // Reuse
    int option = 1;
    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option)) < 0) {
        perror("SO_REUSEADDR failed");

        return 1;
    }

    // Timeout
    // ? Возможно здесь это не надо
    // setTimeout(socket_desc);

    // Bind
    if (bind(serverSocket, (struct sockaddr *) &serverAddr, sizeof(serverAddr)) < 0) {
        perror("Bind failed");

        return 1;
    }
    DEBUG && puts("Bind done");

    runGarbageCollectorThread(&firstByte);

    // Listen
    listen(serverSocket, SOCKET_QUEUE_LENGTH);

    // Accept and incoming connection
    puts("Waiting for incoming connections...");
    sockAddrSize = sizeof(struct sockaddr_in);

    DEBUG && printf("first = %p\n", first);
    DEBUG && printf("&first = %p\n", &first);

    while ((clientSocket = accept(serverSocket, (struct sockaddr *) &clientAddr, (socklen_t *) &sockAddrSize))) {
        if (clientSocket == -1) {
            perror("Accept failed"); // Timeout

            if (serverSocket)
                continue;
            else
                break;
        }

        setTimeout(clientSocket);

        inet_ntop(AF_INET, &(clientAddr.sin_addr), ip, INET_ADDRSTRLEN);
        port = ntohs(clientAddr.sin_port);

        pthread_t sniffer_thread;

        struct args *threadArguments = (struct args *) c_malloc(sizeof(struct args));
        threadArguments->sock = clientSocket;
        threadArguments->number = ++threadCounter;
        threadArguments->sem = &sem;
        threadArguments->first = &first;
        threadArguments->ip = clientAddr.sin_addr.s_addr;
        threadArguments->firstByte = &firstByte;

        DEBUG && printf("Connection accepted: %s:%d sock:%d number:%d\n", ip, port, clientSocket, threadCounter);

        if (pthread_create(&sniffer_thread, NULL, connection_handler, (void *) threadArguments) != 0) {
            perror("Could not create thread");

            return 1;
        }

        DEBUG && puts("Handler assigned");
    }

    if (clientSocket < 0) {
        perror("Accept failed (end)");

        return 1;
    }

    return 0;
}
