#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "queue.h"
#include "sem.h"
#include "alloc.h"

#define PATH 0
#define PARAM 1
#define VALUE 2
#define PATH_SIZE 50
#define PARAM_VALUE_SIZE 20
#define TIMEOUT_SOCKET 60
#define SOCKET_QUEUE_LENGTH 150
#define READ_ONE_LENGTH 2000
#define MAX_MESSAGE_LENGTH 20000
#define MAX_RESULT_LENGTH 20000


/**
 * Сокет сервера
 */
int serverSocket;

/**
 * Семафор для работы с очередью
 */
struct rk_sema sem;

/**
 * Первый элемент очереди
 */
struct queue *first = NULL;

/**
 * Аргументы переданные в поток
 */
struct args {
    int sock;
    int number;
};

// The thread function
void *connection_handler(void *);

void setTimeout(int socket);

void parseUri(char *message);

// Test queue
__unused void testQueue();

int main(int argc, char *argv[]) {
    // Check program's arguments
    if (argc < 2) {
        printf("./server port\n");
        return 1;
    }

    rk_sema_init(&sem, 1);

    // testQueue();

    // Vars
    int clientSocket, sockAddrSize;
    struct sockaddr_in serverAddr, clientAddr;
    char ip[100];
    uint16_t port;
    int threadCounter = 0;

    // Create socket
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1)
        printf("Could not create socket");

    puts("Socket created");

    //Prepare the sockaddr_in structure
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(atoi(argv[1]));

    // Reuse
    int option = 1;
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));

    // Timeout
    // ? Возможно здесь это не надо
    // setTimeout(socket_desc);

    // Bind
    if (bind(serverSocket, (struct sockaddr *) &serverAddr, sizeof(serverAddr)) < 0) {
        perror("Bind failed");
        return 1;
    }
    puts("Bind done");

    // Listen
    listen(serverSocket, SOCKET_QUEUE_LENGTH);

    // Accept and incoming connection
    puts("Waiting for incoming connections...");
    sockAddrSize = sizeof(struct sockaddr_in);

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

        printf("Connection accepted: %s:%d sock:%d number:%d\n", ip, port, clientSocket, threadCounter);

        if (pthread_create(&sniffer_thread, NULL, connection_handler, (void *) threadArguments) != 0) {
            perror("Could not create thread");

            return 1;
        }

        puts("Handler assigned");
    }

    if (clientSocket < 0) {
        perror("Accept failed (end)");

        return 1;
    }

    return 0;
}

/**
 * @param char* start
 * @param char* string
 * @return _Bool Строка str начинается на pre
 */
_Bool startsWith(const char *start, const char *string) {
    size_t lenPre = strlen(start),
            lenStr = strlen(string);
    return lenStr < lenPre ? 0 : memcmp(start, string, lenPre) == 0;
}

/**
 * This will handle connection for each client
 * @param _args
 * @return
 */
void *connection_handler(void *_args) {
    int threadSocket = ((struct args *) _args)->sock;
    int threadNumber = ((struct args *) _args)->number;

    c_free(_args);

    rk_sema_wait(&sem);
    first = addToQueue(first, threadNumber);
    rk_sema_post(&sem);

    printf("Handler: sock:%d number:%d\n", threadSocket, threadNumber);

    int receiveBytesCount;
    _Bool isHttp = 0;
    char fullMessage[MAX_MESSAGE_LENGTH] = {0};
    char readOneMessage[READ_ONE_LENGTH + 1] = {0};
    char resultMessage[MAX_RESULT_LENGTH] = {0};

    while (memset(readOneMessage, 0, sizeof(readOneMessage))
           && (receiveBytesCount = recv(threadSocket, readOneMessage, READ_ONE_LENGTH, 0)) > 0) {
        printf("> %s", readOneMessage);

        if (startsWith("stop", readOneMessage)) {
            printf("STOP\n");
            exit(0);
        }

        if (!isHttp && startsWith("GET ", readOneMessage)) {
            isHttp = 1;
            printf("isHttp = 1\n");
        }

        if (isHttp) {
            if (strlen(fullMessage) + strlen(readOneMessage) > MAX_MESSAGE_LENGTH) {
                printf("Message too long\n");
                break;
            }

            strcat(fullMessage, readOneMessage);
            printf("message = %s", fullMessage);

            if (strstr(fullMessage, "\r\n\r\n") != NULL) {
                printf("Message complete\n");

                parseUri(fullMessage);

                rk_sema_wait(&sem);
                char *data = printQueue(first);
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
        printf("< %s", readOneMessage);
    }

    close(threadSocket);

    rk_sema_wait(&sem);
    first = deleteFromQueue(first, threadNumber);
    rk_sema_post(&sem);

    printf("Recv bytes: %d\n", receiveBytesCount);

    if (receiveBytesCount == 0)
        puts("Client Disconnected");
    else if (receiveBytesCount < 0)
        perror("Recv failed");
    else
        puts("I Disconnect Client");

    if (pthread_detach(pthread_self()) != 0) {
        perror("could not detach thread");
    }

    return 0;
}

/**
 * Установка Timeout
 * @param socket
 */
void setTimeout(int socket) {
    struct timeval tv;
    tv.tv_sec = TIMEOUT_SOCKET;
    tv.tv_usec = 0;
    if (setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, (const char *) &tv, sizeof tv) < 0) {
        printf("Could not set SO_RCVTIMEO socket\n");

        exit(1);
    }
    if (setsockopt(socket, SOL_SOCKET, SO_SNDTIMEO, (const char *) &tv, sizeof tv) < 0) {
        printf("Could not set SO_SNDTIMEO socket\n");

        exit(1);
    }
}

/**
 * Разбор URI строки
 * @param message
 */
void parseUri(char *message) {
    printf("Uri:\n");
    char path[PATH_SIZE + 1] = {0},
            param[PARAM_VALUE_SIZE + 1] = {0},
            value[PARAM_VALUE_SIZE + 1] = {0};
    int status = PATH;
    size_t len;

    for (int i = strlen("GET "); i < strlen(message); i++) {
        char current = message[i];
        if (current == ' ') {
            printf("Param:%s=Value:%s, %lu %lu %d\n", param, value, strlen(param), strlen(value),
                   strcmp(param, "123456"));
            break;
        }
        printf("%d=%c\n", i, current);
        if (status == PATH) {
            if (current == '?') {
                status = PARAM;
                printf("Path:%s, %lu\n", path, strlen(path));
                printf("Status=PARAM\n");
                continue;
            }
            if ((len = strlen(path)) < PATH_SIZE) {
                path[len] = current;
            }
            continue;
        }
        if (current == '=') {
            memset(&value, 0, PARAM_VALUE_SIZE);
            status = VALUE;
            continue;
        }
        if (current == '&') {
            printf("Param:%s=Value:%s, %lu %lu %d\n", param, value, strlen(param), strlen(value),
                   strcmp(param, "123456"));

            memset(&param, 0, PARAM_VALUE_SIZE);
            status = PARAM;

            continue;
        }
        if (status == PARAM && (len = strlen(param)) < PARAM_VALUE_SIZE) {
            param[len] = current;
        }
        if (status == VALUE && (len = strlen(value)) < PARAM_VALUE_SIZE) {
            value[len] = current;
        }
    }
    printf(".\n");

}

/**
 * Тестирует очередь
 */
__unused void testQueue() {
    first = addToQueue(first, 1);
    first = addToQueue(first, 2);
    first = addToQueue(first, 3);
    first = addToQueue(first, 4);
    printQueue(first);

    first = deleteFromQueue(first, 2);
    printQueue(first);

    first = deleteFromQueue(first, 1);
    printQueue(first);

    first = deleteFromQueue(first, 4);
    printQueue(first);

    first = deleteFromQueue(first, 3);
    printQueue(first);

    first = addToQueue(first, 5);
    printQueue(first);

    exit(0);
}
