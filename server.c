#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>

struct args {
    int sock;
    int number;
};

struct queue {
    int n;
    long int t_time;
    struct queue *q;
};

struct queue *first = NULL;

/**
 * Добавляет элемент в очередь
 * @param n
 */
void addToQueue(int n) {
    printf("Start to add n = %d\n", n);
    if (first == NULL) {
        printf("  Add %d: First is null\n", n);
        first = calloc(1, sizeof(struct queue));

        first->n = n;
        first->t_time = time(NULL);

    } else {

        struct queue *next = first;
        while (next->q != NULL) {
            next = next->q;
            printf("  Add %d: Next->n = %d\n", n, next->n);
        }

        struct queue *last = calloc(1, sizeof(struct queue));
        last->n = n;
        last->t_time = time(NULL);
        next->q = last;
    }
    printf("End of add n = %d\n", n);
}

/**
 * Распечатка очереди
 */
char *printQueue() {
    puts("Start printing…");

    char *result, line[20000];
    long int t_time = time(NULL);

    result = calloc(sizeof(char), 20000);
    sprintf(line, "%.24s - now\n", ctime(&t_time));
    strcat(result, line);

    struct queue *next = first;
    while (next != NULL) {
        printf("  Print: n = %d\n", next->n);

        sprintf(line, "%.24s - %d\n", ctime(&next->t_time), next->n);
        strcat(result, line);

        next = next->q;
    }

    puts("End of print");

    return result;
}

/**
 * Удаление элемента из очереди
 * @param n
 */
void deleteFromQueue(int n) {
    printf("Delete: n = %d\n", n);

    struct queue *next = first;
    struct queue *previous = NULL;

    while (next != NULL) {
        printf("  Delete %d: n = %d\n", n, next->n);

        if (next->n == n) {
            printf("  Delete %d: next->n == n\n", n);
            if (previous == NULL) {
                printf("  Delete %d: Previous is null\n", n);

                first = next->q;
                free(next);

                break;
            } else {
                printf("  Delete %d: Previous n = %d\n", n, previous->n);

                previous->q = next->q;
                free(next);

                break;
            }
        }

        previous = next;
        next = next->q;
    }

    printf("End of delete %d", n);
}

// The thread function
void *connection_handler(void *);

int main(int argc, char *argv[]) {
    // Check program's arguments
    if (argc < 2) {
        printf("./server port\n");
        return 1;
    }

/*
    addToQueue(1);
    addToQueue(2);
    addToQueue(3);
    addToQueue(4);
    printQueue();
    deleteFromQueue(2);
    printQueue();
    deleteFromQueue(1);
    printQueue();
    deleteFromQueue(4);
    printQueue();
    deleteFromQueue(3);
    printQueue();
    addToQueue(5);
    printQueue();

    return 0;
*/
    // Vars
    int socket_desc, client_sock, c;
    struct sockaddr_in server, client;
    char ip[100];
    int port;
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

    // Timeout
    struct timeval tv;
    tv.tv_sec = 60;
    tv.tv_usec = 0;
    if (setsockopt(socket_desc, SOL_SOCKET, SO_RCVTIMEO, (const char *) &tv, sizeof tv) < 0) {
        printf("Could not set SO_RCVTIMEO socket");
        return -1;
    }
    if (setsockopt(socket_desc, SOL_SOCKET, SO_SNDTIMEO, (const char *) &tv, sizeof tv) < 0) {
        printf("Could not set SO_SNDTIMEO socket");
        return -1;
    }

    // Bind
    if (bind(socket_desc, (struct sockaddr *) &server, sizeof(server)) < 0) {
        perror("Bind failed");
        return 1;
    }
    puts("Bind done");

    // Listen
    listen(socket_desc, 15);

    // Accept and incoming connection
    puts("Waiting for incoming connections...");
    c = sizeof(struct sockaddr_in);

    while ((client_sock = accept(socket_desc, (struct sockaddr *) &client, (socklen_t * ) & c))) {
        if (client_sock == -1) {
            perror("Accept failed");
            continue;
        }

        inet_ntop(AF_INET, &(client.sin_addr), ip, INET_ADDRSTRLEN);
        port = (int) ntohs(client.sin_port);

        pthread_t sniffer_thread;
        //new_sock = malloc(1);
        //*new_sock = client_sock;

        struct args *_args = (struct args *) malloc(sizeof(struct args));
        _args->sock = client_sock;
        _args->number = ++number;

        printf("Connection accepted: %s:%d sock:%d number:%d\n", ip, port, client_sock, number);

        if (pthread_create(&sniffer_thread, NULL, connection_handler, (void *) _args) < 0) {
            perror("could not create thread");
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
 * @param char* pre
 * @param char* str
 * @return _Bool Строка str начинается на pre
 */
_Bool startsWith(const char *pre, const char *str) {
    size_t lenpre = strlen(pre),
            lenstr = strlen(str);
    return lenstr < lenpre ? 0 : memcmp(pre, str, lenpre) == 0;
}

/**
 * This will handle connection for each client
 * @param socket_desc
 * @return
 */
void *connection_handler(void *_args) {
    // Get the socket descriptor

    int sock = ((struct args *) _args)->sock;
    int number = ((struct args *) _args)->number;

    addToQueue(number);

    free(_args);

    printf("Handler: sock:%d number:%d\n", sock, number);

    int n;
    _Bool isHttp = 0;
    char message[2000] = {0};
    char client_message[2000], resultMessage[20000];

    while (memset(client_message, 0, sizeof(client_message))
           && (n = recv(sock, client_message, 2000, 0)) > 0) {
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
            strcat(message, client_message);
            printf("message = %s", message);

            if (strstr(message, "\r\n\r\n") != NULL) {
                printf("Message complete\n");

                char *data = printQueue();
                int lenData = strlen(data);

                sprintf(resultMessage, "HTTP/1.1 200 OK\r\nContent-Type: text/plain; charset=UTF-8\r\nContent-Length:%d\r\n\r\n%s",
                        lenData, data);
                send(sock, resultMessage, strlen(resultMessage), 0);
                memset(message, 0, sizeof(message));
                free(data);

                break;
            }

            continue;
        }


        send(sock, client_message, n, 0);
        printf("< %s", client_message);
    }
    close(sock);

    deleteFromQueue(number);

    printf("Recv bytes: %d\n", n);

    if (n == 0)
        puts("Client Disconnected");
    else
        perror("recv failed");

    return 0;
}
