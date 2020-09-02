#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sched.h>
#include <unistd.h>
#include "thread.h"
#include "sem.h"
#include "alloc.h"
#include "queue.h"
#include "string.h"
#include "uri.h"
#include "data.h"

#define DEBUG 0
#define KEEP_ALIVE 1
#define READ_LENGTH 2000
#define MESSAGE_LENGTH 20000
#define THREAD_RESULT_LENGTH 20000
#define GARBAGE_COLLECTOR_TIME 15 * 60

struct garbageCollectorArgs {
    struct firstByte *firstByte;
};

void *garbageCollectorThread(void *_args);

void runGarbageCollectorThread(struct firstByte *firstByte) {
    pthread_attr_t tattr;
    pthread_t tid;
    int ret;
    int newPriority = 5;
    struct sched_param param;

    struct garbageCollectorArgs *garbageCollectorArgs = c_calloc(1, sizeof(struct garbageCollectorArgs));
    garbageCollectorArgs->firstByte = firstByte;

    // initialized with default attributes
    ret = pthread_attr_init(&tattr);

    // safe to get existing scheduling param
    ret = pthread_attr_getschedparam(&tattr, &param);

    if(param.sched_priority > newPriority) {
        printf("Garbage thread change priority %d -> %d\n", param.sched_priority, newPriority);

        // set the priority; others are unchanged
        param.sched_priority = newPriority;
    } else {
        printf("Garbage thread NO change priority %d -> %d\n", param.sched_priority, newPriority);
    }


    // setting the new scheduling param
    ret = pthread_attr_setschedparam(&tattr, &param);

    // with new priority specified
    ret = pthread_create(&tid, &tattr, garbageCollectorThread, (void *) garbageCollectorArgs);
}

void *garbageCollectorThread(void *_args) {
    struct firstByte *firstByte = ((struct garbageCollectorArgs *) _args)->firstByte;
    c_free(_args);

    while (1) {
        runGarbageCollector(firstByte);
        sleep(GARBAGE_COLLECTOR_TIME);
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
    in_addr_t ip = ((struct args *) _args)->ip;
    struct firstByte *firstByte = ((struct args *) _args)->firstByte;

    DEBUG && printf("first = %p\n", first);
    DEBUG && printf("*first = %p\n", *first);

    c_free(_args);

    rk_sema_wait(&sem);
    *first = addToQueue(*first, threadNumber);
    rk_sema_post(&sem);

    DEBUG && printf("Handler: sock:%d number:%d\n", threadSocket, threadNumber);

    int receiveBytesCount;
    _Bool isHttp = 0;
    char fullMessage[MESSAGE_LENGTH] = {0};
    char readOneMessage[READ_LENGTH + 1] = {0};
    char resultMessage[THREAD_RESULT_LENGTH] = {0};

    while (memset(readOneMessage, 0, sizeof(readOneMessage))
           && (receiveBytesCount = recv(threadSocket, readOneMessage, READ_LENGTH, 0)) > 0) {
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
            if (strlen(fullMessage) + strlen(readOneMessage) > MESSAGE_LENGTH) {
                printf("Message too long\n");
                break;
            }

            strcat(fullMessage, readOneMessage);
            DEBUG && printf("message = %s", fullMessage);

            if (strstr(fullMessage, "\r\n\r\n") != NULL) {
                DEBUG && printf("Message complete\n");

                struct query query = {0};
                query.ip = ip;
                query.numwant = DEFAULT_NUM_WANT;

                parseUri(&query, fullMessage);

                struct peer *peer;
                struct result result = {0};

                switch (query.event) {
                    case 0:
                        rk_sema_wait(&sem);
                        char *data = printQueue(*first);
                        rk_sema_post(&sem);
                        size_t lenData = strlen(data);

                        sprintf(resultMessage,
                                "HTTP/1.1 200 OK\r\n"
                                "Content-Type: text/plain\r\n"
                                "Content-Length: %zu\r\n"
                                "Server: sc6\r\n"
                                "\r\n"
                                "%s",
                                lenData, data);
                        c_free(data);
                        send(threadSocket, resultMessage, strlen(resultMessage), 0);

                        break;
                    case EVENT_ID_STOPPED:
                        waitSem(firstByte, &query);
                        peer = deletePeer(firstByte, &query);
                        getPeerString(&result, peer, &query);
                        postSem(firstByte, &query);
                        break;
                    default:
                        waitSem(firstByte, &query);
                        peer = updatePeer(firstByte, &query);
                        getPeerString(&result, peer, &query);
                        postSem(firstByte, &query);
                        // runGarbageCollector(firstByte);
                        break;
                } // End of switch

                if (query.event != 0) {
                    sprintf(resultMessage,
                            "HTTP/1.1 200 OK\r\n"
                            "Content-Type: text/plain; charset=UTF-8\r\n"
                            "Content-Length: %zu\r\n"
                            "Server: sc6\r\n"
                            "\r\n",
                            result.size);
                    unsigned long resultMessageSize = strlen(resultMessage);
                    memcpy(&resultMessage[resultMessageSize], result.data, result.size);

                    send(threadSocket, resultMessage, resultMessageSize + result.size, 0);

                    c_free(result.data);
                } // End of query.event != 0

                int canKeepAlive = (strstr(fullMessage, "HTTP/1.1") != NULL);
                memset(fullMessage, 0, sizeof(fullMessage));

                if (KEEP_ALIVE && canKeepAlive)
                    continue; // Connection: Keep-Alive
                else
                    break;
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
