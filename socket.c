#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <string.h>
#include "socket.h"

#define DEBUG 0
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

/**
 * Отправка сообщения по сокету
 * @param socket
 * @param code
 * @param message
 * @param size
 */
void sendMessage(int socket, int code, char *message, size_t size, int canKeepAlive) {
    char headers[1000] = {0};
    char body[1000] = {0};

    // First line headers
    switch (code) {
        case 400:
            sprintf(headers, "HTTP/1.0 400 Invalid Request\r\n");
            break;
        case 404:
            sprintf(headers, "HTTP/1.0 404 Not Found\r\n");
            break;
        case 200:
        default:
            sprintf(headers, "HTTP/1.1 200 OK\r\n");
            break;
    }
    send(socket, headers, strlen(headers), 0);
    DEBUG && printf("%s", headers);

    if (code != 200) {
        sprintf(body,
                "d"
                "14:failure reason"
                "%zu:%s"
                "e",
                size,
                message
        );

        size = strlen(body);
    }

    // End of headers
    if (canKeepAlive) {
        memset(headers, 0, sizeof(headers));
        sprintf(headers, "Connection: Keep-Alive\r\n");
        send(socket, headers, strlen(headers), 0);
        DEBUG && printf("%s", headers);
    }

    memset(headers, 0, sizeof(headers));
    sprintf(headers, "Content-Type: text/plain\r\n"
                     "Content-Length: %zu\r\n"
                     "Server: sc6\r\n"
                     "\r\n",
            size
    );
    send(socket, headers, strlen(headers), 0);
    DEBUG && printf("%s", headers);

    // Body
    if (code == 200) {
        send(socket, message, size, 0);
        DEBUG && printf("%s\n", message);
    } else {
        send(socket, body, strlen(body), 0);
        DEBUG && printf("%s", body);
    }
}
