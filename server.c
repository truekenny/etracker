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
int socket_desc;

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
    int client_sock, c;
    struct sockaddr_in server, client;
    char ip[100];
    uint16_t port;
    int number = 0;

    // Create socket
    socket_desc = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_desc == -1)
        printf("Could not create socket");

    puts("Socket created");

    //Prepare the sockaddr_in structure
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(atoi(argv[1]));

    // Reuse
    int option = 1;
    setsockopt(socket_desc, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));

    // Timeout
    setTimeout(socket_desc);

    // Bind
    if (bind(socket_desc, (struct sockaddr *) &server, sizeof(server)) < 0) {
        perror("Bind failed");
        return 1;
    }
    puts("Bind done");

    // Listen
    listen(socket_desc, SOCKET_QUEUE_LENGTH);

    // Accept and incoming connection
    puts("Waiting for incoming connections...");
    c = sizeof(struct sockaddr_in);

    while ((client_sock = accept(socket_desc, (struct sockaddr *) &client, (socklen_t *) &c))) {
        if (client_sock == -1) {
            perror("Accept failed"); // Timeout

            if (socket_desc)
                continue;
            else
                break;
        }

        setTimeout(client_sock);

        inet_ntop(AF_INET, &(client.sin_addr), ip, INET_ADDRSTRLEN);
        port = ntohs(client.sin_port);

        pthread_t sniffer_thread;

        struct args *_args = (struct args *) c_malloc(sizeof(struct args));
        _args->sock = client_sock;
        _args->number = ++number;

        printf("Connection accepted: %s:%d sock:%d number:%d\n", ip, port, client_sock, number);

        if (pthread_create(&sniffer_thread, NULL, connection_handler, (void *) _args) != 0) {
            perror("Could not create thread");

            return 1;
        }

        puts("Handler assigned");
    }

    if (client_sock < 0) {
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
 * @param socket_desc
 * @return
 */
void *connection_handler(void *_args) {
    int sock = ((struct args *) _args)->sock;
    int number = ((struct args *) _args)->number;

    c_free(_args);

    rk_sema_wait(&sem);
    first = addToQueue(first, number);
    rk_sema_post(&sem);

    printf("Handler: sock:%d number:%d\n", sock, number);

    int n;
    _Bool isHttp = 0;
    char message[MAX_MESSAGE_LENGTH] = {0};
    char client_message[READ_ONE_LENGTH + 1] = {0};
    char resultMessage[MAX_RESULT_LENGTH] = {0};

    while (memset(client_message, 0, sizeof(client_message))
           && (n = recv(sock, client_message, READ_ONE_LENGTH, 0)) > 0) {
        printf("> %s", client_message);

        if (startsWith("stop", client_message)) {
            printf("STOP\n");
            exit(0);
        }

        if (!isHttp && startsWith("GET ", client_message)) {
            isHttp = 1;
            printf("isHttp = 1\n");
        }

        if (isHttp) {
            if (strlen(message) + strlen(client_message) > MAX_MESSAGE_LENGTH) {
                printf("Message too long\n");
                break;
            }

            strcat(message, client_message);
            printf("message = %s", message);

            if (strstr(message, "\r\n\r\n") != NULL) {
                printf("Message complete\n");

                parseUri(message);

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
                send(sock, resultMessage, strlen(resultMessage), 0);
                memset(message, 0, sizeof(message));
                c_free(data);

                break;
            }

            continue;
        }


        send(sock, client_message, n, 0);
        printf("< %s", client_message);
    }

    close(sock);

    rk_sema_wait(&sem);
    first = deleteFromQueue(first, number);
    rk_sema_post(&sem);

    printf("Recv bytes: %d\n", n);

    if (n == 0)
        puts("Client Disconnected");
    else if (n < 0)
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
