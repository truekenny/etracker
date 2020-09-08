#include <stddef.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/event.h>
#include <assert.h>
#include <memory.h>
#include "socket_tcp.h"
#include "thread_client_tcp.h"
#include "socket.h"
#include "alloc.h"
#include "request.h"
#include "string.h"

#define DEBUG 0
#define DEBUG_KQUEUE 0
#define SOCKET_QUEUE_LENGTH 150
#define EVENTS_EACH_LOOP 32

struct request
        *firstRequest = {0},
        *lastRequest = {0};

void *serverTcpHandler(void *args) {
    struct stats *stats = ((struct serverTcpArgs *) args)->stats;
    struct rk_sema *semaphoreQueue = ((struct serverTcpArgs *) args)->semaphoreQueue;
    struct queue **queue = ((struct serverTcpArgs *) args)->queue;
    struct firstByteData *firstByteData = ((struct serverTcpArgs *) args)->firstByteData;
    char *port = ((struct serverTcpArgs *) args)->port;
    c_free(args);

    pthread_cond_t signalRequest = PTHREAD_COND_INITIALIZER;
    pthread_mutex_t mutexSignalRequest = PTHREAD_MUTEX_INITIALIZER;
    struct rk_sema semaphoreRequest = {0};
    rk_sema_init(&semaphoreRequest, 1);


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

    int kQueue = kqueue();

    pthread_t tcpClientThread;
    // Кол-во воркеров = кол-ву ядер
    for (int threadNumber = 0; threadNumber < sysconf(_SC_NPROCESSORS_ONLN); threadNumber++) {
        struct clientTcpArgs *clientTcpArgs = (struct clientTcpArgs *) c_malloc(sizeof(struct clientTcpArgs));
        clientTcpArgs->threadNumber = threadNumber;
        clientTcpArgs->semaphoreQueue = semaphoreQueue;
        clientTcpArgs->queue = queue;
        clientTcpArgs->firstByteData = firstByteData;
        clientTcpArgs->stats = stats;

        clientTcpArgs->signalRequest = &signalRequest;
        clientTcpArgs->mutexSignalRequest = &mutexSignalRequest;
        clientTcpArgs->firstRequest = &firstRequest;
        clientTcpArgs->lastRequest = &lastRequest;
        clientTcpArgs->semaphoreRequest = &semaphoreRequest;

        if (pthread_create(&tcpClientThread, NULL, clientTcpHandler, (void *) clientTcpArgs) != 0) {
            perror("Could not create TCP thread");

            exit(5);
        }    // end of workers
    }

    // Listen
    listen(serverSocket, SOCKET_QUEUE_LENGTH);

    puts("Waiting TCP for incoming connections...");

    struct kevent kEvent;
    EV_SET(&kEvent, serverSocket, EVFILT_READ, EV_ADD, 0, 0, NULL);
    assert(-1 != kevent(kQueue, &kEvent, 1, NULL, 0, NULL));

    struct kevent evList[EVENTS_EACH_LOOP];

    while (1) {
        int nev = kevent(kQueue, NULL, 0, evList, EVENTS_EACH_LOOP, NULL);

        DEBUG_KQUEUE && printf("socket_tcp.c: go nev=%d\n", nev);

        for (int index = 0; index < nev; index++) {
            int currentSocket = (int) evList[index].ident;

            if (evList[index].flags & EV_EOF) {
                DEBUG_KQUEUE && printf("socket_tcp.c: Disconnect\n");

                close(currentSocket);
                // Socket is automatically removed from the kq by the kernel.
            } else if (currentSocket == serverSocket) {
                struct sockaddr_in addr;
                socklen_t socklen = sizeof(addr);
                int connfd = accept(currentSocket, (struct sockaddr *) &addr, &socklen);
                if (connfd == -1) {
                    printf("Socket %d\n", currentSocket);
                    perror("Accept error");
                    continue;
                }

                // Listen on the new socket
                EV_SET(&kEvent, connfd, EVFILT_READ, EV_ADD | EV_CLEAR, 0, 0, NULL);
                kevent(kQueue, &kEvent, 1, NULL, 0, NULL);
                DEBUG_KQUEUE && printf("socket_tcp.c: Got connection!\n");

                int flags = fcntl(connfd, F_GETFL, 0);
                if (flags < 0)
                    perror("Flags failed");
                fcntl(connfd, F_SETFL, flags | O_NONBLOCK);
            } else if (evList[index].filter == EVFILT_READ) {
                DEBUG_KQUEUE && printf("socket_tcp.c: Read %d\n", currentSocket);
                // Read from socket.
                char readBuffer[1024];
                memset(readBuffer, 0, sizeof(readBuffer));
                size_t readSize = recv(currentSocket, readBuffer, sizeof(readBuffer), MSG_NOSIGNAL | MSG_PEEK);
                DEBUG_KQUEUE && printf("socket_tcp.c: read %zu bytes fd=%d \n", readSize, currentSocket);

                if (strstr(readBuffer, "\r\n\r\n") != NULL) {
                    struct block *readBlock = initBlock();
                    addStringBlock(readBlock, readBuffer, readSize);

                    rk_sema_wait(&semaphoreRequest);
                    addRequest(&firstRequest, &lastRequest, currentSocket, readBlock);
                    rk_sema_post(&semaphoreRequest);

                    // Сброс буфера, поскольку запрос полный, прочитать столько сколько было пикнуто
                    recv(currentSocket, readBuffer, readSize, MSG_NOSIGNAL);

                    // SIGNAL
                    DEBUG_KQUEUE && printf("socket_tcp.c: Signal Read %d\n", currentSocket);
                    pthread_mutex_lock(&mutexSignalRequest);
                    pthread_cond_signal(&signalRequest);
                    pthread_mutex_unlock(&mutexSignalRequest);
                }

            }
        }

        // Чтобы нормально работала подсветка кода в IDE
        if (rand() % 2 == 3) break;
    }

    puts("TCP server socket finished");

    return NULL;
}
