#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include "socket_udp.h"
#include "alloc.h"

#define DEBUG 1
#define RECEIVED_UDP_MESSAGE_LENGTH 200
#define MSG_CONFIRM 0

#define PROTOCOL_ID 0x8019102717040000

#define ACTION_CONNECT 0
#define ACTION_ANNOUNCE 1

#define EVENT_NONE 0
#define EVENT_COMPLETED 1
#define EVENT_STARTED 2
#define EVENT_STOPPED 3
#define EVENT_PAUSED 4

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
};

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
};

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
    unsigned char info_hash[20];
    unsigned char peer_id[20];
    unsigned long downloaded;
    unsigned long left;
    unsigned long uploaded;
    unsigned int event;
    unsigned int ip;
    unsigned int key;
    unsigned int num_want;
    unsigned short port;
};

/*
0           32-bit integer  action          1 // announce
4           32-bit integer  transaction_id
8           32-bit integer  interval
12          32-bit integer  leechers
16          32-bit integer  seeders
20 + 6 * n  32-bit integer  IP address
24 + 6 * n  16-bit integer  TCP port
26
 */

struct announceResponse {
    unsigned int action;
    unsigned int transaction_id;
    unsigned int interval;
    unsigned int leechers;
    unsigned int seeders;
};

struct announcePeerResponse {
    unsigned int ip;
    unsigned short port;
};

void *serverUdpHandler(void *args) {
    struct stats *stats = ((struct serverUdpArgs *) args)->stats;
    struct firstByte *firstByte = ((struct serverUdpArgs *) args)->firstByte;
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
    unsigned long connectionCount = 0;
    struct connectResponse connectResponse = {0}; // ACTION_CONNECT = 0

    while ((receivedSize = recvfrom(serverSocket, (char *) receivedMessage, announceRequestSize,
                                    MSG_WAITALL, (struct sockaddr *) &clientAddr,
                                    (socklen_t *) &sockAddrSize)) > 0) {

        // receivedMessage[receivedSize] = '\0';
        // printf("Client : %s\n", receivedMessage);

        if (receivedSize == connectRequestSize) {
            struct connectRequest *connectRequest = (struct connectRequest *) receivedMessage;
            if (connectRequest->protocol_id == PROTOCOL_ID && connectRequest->action == ACTION_CONNECT) {
                connectResponse.transaction_id = connectRequest->transaction_id;
                connectResponse.connection_id = ++connectionCount;

                sendto(serverSocket, (const char *) &connectResponse, connectResponseSize,
                       MSG_CONFIRM, (const struct sockaddr *) &clientAddr,
                       sockAddrSize);
            }
        } else if (receivedSize == announceRequestSize) {
            struct announceRequest *announceRequest = (struct announceRequest *) receivedMessage;
            announceRequest->action = htonl(announceRequest->action);
            if (announceRequest->action == ACTION_ANNOUNCE) {
                announceRequest->event = htonl(announceRequest->event);
                announceRequest->num_want = htonl(announceRequest->num_want);
                // announceRequest->port = htons(announceRequest->port);

                // Аргументы потока
                // Поток
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
        stats->recv_failed++;
        if (DEBUG) perror("Recv failed");
    } else {
        DEBUG && puts("I Disconnect Client");
    }


    puts("UDP server socket finished");

    return NULL;
}
