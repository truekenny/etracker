#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <pthread.h>
#include "socket_udp.h"
#include "alloc.h"
#include "thread_client_udp.h"
#include "socket_udp_response_structure.h"
#include "string.h"

#define DEBUG 0
// Размер заголовка пакета scrape + 74 x info_hash (по протоколу это максимальное кол-во)
#define RECEIVED_UDP_MESSAGE_LENGTH 1496
#define MSG_CONFIRM_ 0

#define PROTOCOL_ID 0x8019102717040000

/*
0       64-bit integer  protocol_id     0x41727101980 // magic constant
8       32-bit integer  action          0 // connect
12      32-bit integer  transaction_id
16
 */

struct connectRequest {
    unsigned long protocol_id;
    unsigned int action;
    unsigned int transaction_id;
} __attribute__((packed)); // 16

/*
0       32-bit integer  action          0 // connect
4       32-bit integer  transaction_id
8       64-bit integer  connection_id
16
 */

struct connectResponse {
    unsigned int action;
    unsigned int transaction_id;
    unsigned long connection_id;
} __attribute__((packed)); // 16

/*
0       64-bit integer  connection_id
8       32-bit integer  action          1 // announce
12      32-bit integer  transaction_id
16      20-byte string  info_hash
36      20-byte string  peer_id
56      64-bit integer  downloaded
64      64-bit integer  left
72      64-bit integer  uploaded
80      32-bit integer  event           0 // 0: none; 1: completed; 2: started; 3: stopped
84      32-bit integer  IP address      0 // default
88      32-bit integer  key
92      32-bit integer  num_want        -1 // default
96      16-bit integer  port
98
 */

struct announceRequest {
    unsigned long connection_id;
    unsigned int action;
    unsigned int transaction_id;
    unsigned char info_hash[PARAM_VALUE_LENGTH];
    unsigned char peer_id[PARAM_VALUE_LENGTH];
    unsigned long downloaded;
    unsigned long left;
    unsigned long uploaded;
    unsigned int event;
    unsigned int ip;
    unsigned int key;
    unsigned int num_want;
    unsigned short port;
} __attribute__((packed)); // 98

/*
0               64-bit integer  connection_id
8               32-bit integer  action          2 // scrape
12              32-bit integer  transaction_id
16
 */

struct scrapeRequest {
    unsigned long connection_id;
    unsigned int action;
    unsigned int transaction_id;
} __attribute__ ((packed)); // 16

void checkSize() {
    if (sizeof(struct connectRequest) != 16) {
        printf("Failed packed: sizeof(struct connectRequest) != 16\n");
        exit(204);
    }

    if (sizeof(struct connectResponse) != 16) {
        printf("Failed packed: sizeof(struct connectResponse) != 16\n");
        exit(205);
    }

    if (sizeof(struct announceRequest) != 98) {
        printf("Failed packed: sizeof(struct announceRequest) != 98\n");
        exit(206);
    }

    if (sizeof(struct announceHeadResponse) != 20) {
        printf("Failed packed: sizeof(struct announceHeadResponse) != 20\n");
        exit(207);
    }

    if (sizeof(struct announcePeerResponse) != 6) {
        printf("Failed packed: sizeof(struct announcePeerResponse) != 6\n");
        exit(208);
    }
}

void *serverUdpHandler(void *args) {
    checkSize();

    struct stats *stats = ((struct serverUdpArgs *) args)->stats;
    struct firstByteData *firstByte = ((struct serverUdpArgs *) args)->firstByteData;
    char *serverPort = ((struct serverUdpArgs *) args)->port;
    c_free(args);

    int serverSocket;
    char receivedMessage[RECEIVED_UDP_MESSAGE_LENGTH + 1];
    struct sockaddr_in serverAddr, clientAddr;

    // Creating socket file descriptor
    if ((serverSocket = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Socket UDP creation failed");

        exit(201);
    }

    memset(&serverAddr, 0, sizeof(serverAddr));
    memset(&clientAddr, 0, sizeof(clientAddr));

    // Filling server information
    serverAddr.sin_family = AF_INET; // IPv4
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(atoi(serverPort));

    // Bind the socket with the server address
    if (bind(serverSocket, (const struct sockaddr *) &serverAddr, sizeof(serverAddr)) < 0) {
        perror("Bind UDP failed");

        exit(202);
    }

    int sockAddrSize, receivedSize;

    puts("Waiting UDP for incoming connections...");
    sockAddrSize = sizeof(struct sockaddr_in);

    unsigned char connectRequestSize = sizeof(struct connectRequest);
    unsigned char connectResponseSize = sizeof(struct connectResponse);
    unsigned char announceRequestSize = sizeof(struct announceRequest);
    unsigned char scrapeRequestSize = sizeof(struct scrapeRequest);
    unsigned long receiveCount = 0;
    struct connectResponse connectResponse = {}; // ACTION_CONNECT = 0

    while ((receivedSize = recvfrom(serverSocket, (char *) receivedMessage, RECEIVED_UDP_MESSAGE_LENGTH,
                                    MSG_WAITALL, (struct sockaddr *) &clientAddr,
                                    (socklen_t *) &sockAddrSize)) > 0) {
        stats->recv_pass_udp++;
        stats->recv_bytes_udp += receivedSize;

        receiveCount++;

        DEBUG && printHex(receivedMessage, receivedSize);

        struct announceRequest *announceRequest = (struct announceRequest *) receivedMessage;
        struct scrapeRequest *scrapeRequest = (struct scrapeRequest *) receivedMessage;

        if (receivedSize == connectRequestSize) {
            DEBUG && printf("UDP Connect\n");
            stats->connect_udp++;

            struct connectRequest *connectRequest = (struct connectRequest *) receivedMessage;
            if (connectRequest->protocol_id == PROTOCOL_ID && connectRequest->action == ACTION_CONNECT) {
                connectResponse.transaction_id = connectRequest->transaction_id;
                connectResponse.connection_id = receiveCount;

                stats->sent_bytes_udp += connectResponseSize;
                DEBUG && printHex((char *) &connectResponse, connectResponseSize);
                if (sendto(serverSocket, (const char *) &connectResponse, connectResponseSize,
                           MSG_CONFIRM_, (const struct sockaddr *) &clientAddr,
                           sockAddrSize) == -1) {
                    stats->send_failed_udp++;
                } else {
                    stats->send_pass_udp++;
                }
            }
        } else if (receivedSize >= announceRequestSize && htonl(announceRequest->action) == ACTION_ANNOUNCE) {
            DEBUG && printf("UDP Announce\n");
            stats->announce_udp++;

            if (0 && DEBUG) {
                printf("connection_id = %lu \n", announceRequest->connection_id);
                printf("action = %u \n", announceRequest->action);
                printf("transaction_id = %u \n", announceRequest->transaction_id);
                printf("downloaded = %lu \n", announceRequest->downloaded);
                printf("left = %lu \n", announceRequest->left);
                printf("uploaded = %lu \n", announceRequest->uploaded);
                printf("event = %u \n", announceRequest->event);
                printf("ip = %u \n", announceRequest->ip);
                printf("key = %u \n", announceRequest->key);
                printf("num_want = %u \n", announceRequest->num_want);
                printf("port = %u \n", announceRequest->port);
            }

            // Аргументы потока
            struct query *query = c_calloc(1, sizeof(struct query));
            query->udp = 1;
            query->port = announceRequest->port;
            query->event = htonl(announceRequest->event);;
            memcpy(query->info_hash, announceRequest->info_hash, PARAM_VALUE_LENGTH);
            memcpy(query->peer_id, announceRequest->peer_id, PARAM_VALUE_LENGTH);
            query->numwant = htonl(announceRequest->num_want);
            query->ip = clientAddr.sin_addr.s_addr;

            struct clientUdpArgs *clientUdpArgs = c_calloc(1, sizeof(struct clientUdpArgs));
            clientUdpArgs->firstByteData = firstByte;
            clientUdpArgs->stats = stats;
            clientUdpArgs->receiveCount = receiveCount;
            clientUdpArgs->serverSocket = serverSocket;
            clientUdpArgs->transaction_id = announceRequest->transaction_id;
            clientUdpArgs->query = query;
            clientUdpArgs->clientAddr = c_calloc(1, sizeof(struct sockaddr_in));
            memcpy(clientUdpArgs->clientAddr, &clientAddr, sockAddrSize);

            // Поток
            pthread_t udpClientThread;
            if (pthread_create(&udpClientThread, NULL, clientUdpHandler, (void *) clientUdpArgs) != 0) {
                perror("Could not create UDP announce thread");

                exit(203);
            }

        } else if (receivedSize > scrapeRequestSize && htonl(scrapeRequest->action) == ACTION_SCRAPE) {
            DEBUG && printf("UDP Scrape\n");
            stats->scrape_udp++;

            unsigned int hashCount = (receivedSize - sizeof(struct scrapeRequest)) / PARAM_VALUE_LENGTH;
            DEBUG && printf("Hashes = %d\n", hashCount);

            struct block *hashes = initBlock();
            addStringBlock(hashes,
                           &((char *) scrapeRequest)[sizeof(struct scrapeRequest)],
                           hashCount * PARAM_VALUE_LENGTH);

            struct clientUdpArgs *clientUdpArgs = c_calloc(1, sizeof(struct clientUdpArgs));
            clientUdpArgs->firstByteData = firstByte;
            clientUdpArgs->stats = stats;
            clientUdpArgs->receiveCount = receiveCount;
            clientUdpArgs->serverSocket = serverSocket;
            clientUdpArgs->transaction_id = announceRequest->transaction_id;
            clientUdpArgs->clientAddr = c_calloc(1, sizeof(struct sockaddr_in));
            memcpy(clientUdpArgs->clientAddr, &clientAddr, sockAddrSize);
            clientUdpArgs->hashes = hashes;

            // Поток
            pthread_t udpClientThread;
            if (pthread_create(&udpClientThread, NULL, clientUdpScrapeHandler, (void *) clientUdpArgs) != 0) {
                perror("Could not create UDP scrape thread");

                exit(209);
            }
        }

/*
        sendto(serverSocket, (const char *) receivedMessage, receivedSize,
               MSG_CONFIRM, (const struct sockaddr *) &clientAddr,
               sockAddrSize);
*/
    }

    if (receivedSize == 0) {
        DEBUG && puts("Client Disconnected");
    } else if (receivedSize < 0) {
        stats->recv_failed_udp++;
        if (DEBUG) perror("Recv failed");
    } else {
        DEBUG && puts("I Disconnect Client");
    }


    puts("UDP server socket finished");

    return NULL;
}
