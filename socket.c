#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <string.h>
#include "socket.h"
#include "block.h"

#define DEBUG 0

/**
 * Рендер сообщения по сокету
 * @param request
 * @param code
 * @param message
 * @param size
 */
void renderHttpMessage(struct block *block, int code, char *message, size_t size, int canKeepAlive, struct stats *stats) {
    struct block *body = {0};

    // First line headers
    switch (code) {
        case 400:
            stats->http_400++;
            addStringBlock(block, "HTTP/1.0 400 Invalid Request\r\n", 30);
            break;
        case 403:
            stats->http_403++;
            addStringBlock(block, "HTTP/1.0 403 Forbidden\r\n", 24);
            break;
        case 404:
            stats->http_404++;
            addStringBlock(block, "HTTP/1.0 404 Not Found\r\n", 24);
            break;
        case 405:
            stats->http_405++;
            addStringBlock(block, "HTTP/1.0 405 Method Not Allowed\r\n", 33);
            break;
        case 408:
            stats->http_408++;
            addStringBlock(block, "HTTP/1.0 408 Request Timeout\r\n", 30);
            break;
        case 413:
            stats->http_413++;
            addStringBlock(block, "HTTP/1.0 413 Request Entity Too Large\r\n", 39);
            break;
        case 200:
        default:
            stats->http_200++;
            addStringBlock(block, "HTTP/1.1 200 OK\r\n", 17);
            break;
    }

    if (code != 200) {
        body = initBlock();
        addFormatStringBlock(body, 1000,
                             "d"
                             "14:failure reason"
                             "%zu:%s"
                             "e",
                             size,
                             message
        );
        size = body->size;
    }

    // End of headers
    if (canKeepAlive) {
        addStringBlock(block, "Connection: Keep-Alive\r\n", 24);
    }

    addFormatStringBlock(block, 1000, "Content-Type: text/plain\r\n"
                                      "Content-Length: %zu\r\n"
                                      "Server: sc6\r\n"
                                      "\r\n",
                         size
    );

    // Body
    if (code == 200) {
        addStringBlock(block, message, size);
    } else {
        addStringBlock(block, body->data, body->size);
        freeBlock(body);
    }

    // ssize_t result = send_(socket, block->data, block->size, stats);

    DEBUG && printf("%s\n", message);
    //freeBlock(block);
}

ssize_t send_(int socket, void *message, size_t size, struct stats *stats) {
    stats->sent_bytes += size;

    ssize_t result = send(socket, message, size, MSG_DONTWAIT | MSG_NOSIGNAL);
    if (result == -1) {
        stats->send_failed++;
        if (DEBUG) perror("Send failed");
    } else
        stats->send_pass++;

    return result;
}
