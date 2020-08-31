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

#define DEBUG 0
#define SOCKET_QUEUE_LENGTH 150
#define READ_ONE_LENGTH 2000
#define MAX_MESSAGE_LENGTH 20000
#define MAX_RESULT_LENGTH 20000

/**
 * Аргументы переданные в поток
 */
struct args {
    int sock;
    int number;
    struct rk_sema *sem;
    struct queue **first;
};

// The thread function
void *connection_handler(void *);

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

/**
 * This will handle connection for each client
 * @param _args
 * @return
 */
void *connection_handler(void *_args) {
    int threadSocket = ((struct args *) _args)->sock;
    int threadNumber = ((struct args *) _args)->number;
    struct rk_sema sem = *((struct args *) _args)->sem;
    struct queue **first = ((struct args *) _args)->first;
    DEBUG && printf("first = %p\n", first);
    DEBUG && printf("*first = %p\n", *first);

    c_free(_args);

    rk_sema_wait(&sem);
    *first = addToQueue(*first, threadNumber);
    rk_sema_post(&sem);

    DEBUG && printf("Handler: sock:%d number:%d\n", threadSocket, threadNumber);

    int receiveBytesCount;
    _Bool isHttp = 0;
    char fullMessage[MAX_MESSAGE_LENGTH] = {0};
    char readOneMessage[READ_ONE_LENGTH + 1] = {0};
    char resultMessage[MAX_RESULT_LENGTH] = {0};

    while (memset(readOneMessage, 0, sizeof(readOneMessage))
           && (receiveBytesCount = recv(threadSocket, readOneMessage, READ_ONE_LENGTH, 0)) > 0) {
        DEBUG && printf("> %s", readOneMessage);

        if (startsWith("stop", readOneMessage)) {
            printf("STOP\n");
            exit(0);
        }

        if (!isHttp && startsWith("GET ", readOneMessage)) {
            isHttp = 1;
            DEBUG && printf("isHttp = 1\n");
        }

        if (isHttp) {
            if (strlen(fullMessage) + strlen(readOneMessage) > MAX_MESSAGE_LENGTH) {
                printf("Message too long\n");
                break;
            }

            strcat(fullMessage, readOneMessage);
            DEBUG && printf("message = %s", fullMessage);

            if (strstr(fullMessage, "\r\n\r\n") != NULL) {
                DEBUG && printf("Message complete\n");

                parseUri(fullMessage);

                rk_sema_wait(&sem);
                char *data = printQueue(*first);
                rk_sema_post(&sem);
                size_t lenData = strlen(data);

                sprintf(resultMessage,
                        "HTTP/1.1 200 OK\r\n"
                        "Content-Type: text/plain; charset=UTF-8\r\n"
                        "Content-Length: %zu\r\n"
                        "\r\n"
                        "%s",
                        lenData, data);
                send(threadSocket, resultMessage, strlen(resultMessage), 0);
                memset(fullMessage, 0, sizeof(fullMessage));
                c_free(data);

                break;
                // continue; // Connection: Keep-Alive
            }

            continue;
        }


        send(threadSocket, readOneMessage, receiveBytesCount, 0);
        DEBUG && printf("< %s", readOneMessage);
    }

    close(threadSocket);

    rk_sema_wait(&sem);
    *first = deleteFromQueue(*first, threadNumber);
    rk_sema_post(&sem);

    DEBUG && printf("Recv bytes: %d\n", receiveBytesCount);

    if (receiveBytesCount == 0)
        DEBUG && puts("Client Disconnected");
    else if (receiveBytesCount < 0)
        perror("Recv failed");
    else
        DEBUG && puts("I Disconnect Client");

    if (pthread_detach(pthread_self()) != 0) {
        perror("Could not detach thread");
    }

    return 0;
}
