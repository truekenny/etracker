#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <string.h>
#include "socket.h"

#define TIMEOUT_SOCKET 60

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

void sendMessage(int socket, int code, char *message, size_t size) {
    char sendMessage[1000] = {0};

    // First line headers
    switch (code) {
        case 400:
            sprintf(sendMessage, "HTTP/1.0 400 Invalid Request\r\n");
            break;
        case 404:
            sprintf(sendMessage, "HTTP/1.0 404 Not Found\r\n");
            break;
        case 200:
        default:
            sprintf(sendMessage, "HTTP/1.1 200 OK\r\n");
            break;
    }
    send(socket, sendMessage, strlen(sendMessage), 0);

    // End of headers
    memset(sendMessage,0,sizeof(sendMessage));
    sprintf(sendMessage, "Content-Type: text/plain\r\n"
                         "Content-Length: %zu\r\n"
                         "Server: sc6\r\n"
                         "\r\n",
            size
    );
    send(socket, sendMessage, strlen(sendMessage), 0);
    send(socket, message, size, 0);
}
