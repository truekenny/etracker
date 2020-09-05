#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include "socket_udp.h"
#include "alloc.h"

#define DEBUG 1
#define RECEIVED_UDP_MESSAGE_LENGTH 2000
#define MSG_CONFIRM 0

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

    while((receivedSize = recvfrom(serverSocket, (char *) receivedMessage, RECEIVED_UDP_MESSAGE_LENGTH,
                                   MSG_WAITALL, (struct sockaddr *) &clientAddr,
                                   (socklen_t *) &sockAddrSize)) > 0) {

        // receivedMessage[receivedSize] = '\0';
        // printf("Client : %s\n", receivedMessage);

        sendto(serverSocket, (const char *) receivedMessage, receivedSize,
               MSG_CONFIRM, (const struct sockaddr *) &clientAddr,
               sockAddrSize);
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
