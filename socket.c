#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <string.h>
#include "socket.h"

#define DEBUG 0
#define TIMEOUT_SOCKET 5

#ifdef __APPLE__
#define MSG_NOSIGNAL 0
#endif

void send_(int socket, void *message, size_t size);

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

        exit(30);
    }
    if (setsockopt(socket, SOL_SOCKET, SO_SNDTIMEO, (const char *) &tv, sizeof tv) < 0) {
        printf("Could not set SO_SNDTIMEO socket\n");

        exit(31);
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
    send_(socket, headers, strlen(headers));
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

    /*
GET /announce?port=123&peer_id=12345678901234567890&info_hash=12345678901234567890&compact=1 HTTP/1.1
Connection: Keep-Alive
     */


    // End of headers
    if (canKeepAlive) {
        memset(headers, 0, sizeof(headers));
        sprintf(headers, "Connection: Keep-Alive\r\n");
        send_(socket, headers, strlen(headers));
        DEBUG && printf("%s", headers);
    }

    memset(headers, 0, sizeof(headers));
    sprintf(headers, "Content-Type: text/plain\r\n"
                     "Content-Length: %zu\r\n"
                     "Server: sc6\r\n"
                     "\r\n",
            size
    );
    send_(socket, headers, strlen(headers));
    DEBUG && printf("%s", headers);

    // Body
    if (code == 200) {
        send_(socket, message, size);
        DEBUG && printf("%s\n", message);
    } else {
        send_(socket, body, strlen(body));
        DEBUG && printf("%s", body);
    }
}

void send_(int socket, void *message, size_t size) {
    if (send(socket, message, size, MSG_DONTWAIT | MSG_NOSIGNAL) == -1) {
        perror("Send failed");
    }
}
