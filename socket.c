#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>
#include "socket.h"
#include "block.h"
#include "string.h"

/**
 * Рендер сообщения по сокету
 * @param request
 * @param code
 * @param message
 * @param size
 */
void renderHttpMessage(struct render *render) {
    struct block *body = {0};

    // First line headers
    switch (render->code) {
        case 101:
            render->stats->http_101++;
            addFormatStringBlock(render->block, 1000,
                                 "HTTP/1.1 101 Switching Protocols\r\n"
                                 "Upgrade: websocket\r\n"
                                 "Connection: Upgrade\r\n"
                                 "Sec-WebSocket-Accept: %s\r\n",
                                 render->websocketKey);
            break;
        case 400:
            render->stats->http_400++;
            addStringBlock(render->block, "HTTP/1.0 400 Invalid Request\r\n", 30);
            break;
        case 401:
            render->stats->http_401++;
            addStringBlock(render->block, "HTTP/1.0 401 Unauthorized\r\n", 27);
            addStringBlock(render->block, "WWW-Authenticate: Basic realm=\"etracker\"\r\n", 42);
            break;
        case 403:
            render->stats->http_403++;
            addStringBlock(render->block, "HTTP/1.0 403 Forbidden\r\n", 24);
            break;
        case 404:
            render->stats->http_404++;
            addStringBlock(render->block, "HTTP/1.0 404 Not Found\r\n", 24);
            break;
        case 405:
            render->stats->http_405++;
            addStringBlock(render->block, "HTTP/1.0 405 Method Not Allowed\r\n", 33);
            break;
        case 408:
            render->stats->http_408++;
            addStringBlock(render->block, "HTTP/1.0 408 Request Timeout\r\n", 30);
            break;
        case 413:
            render->stats->http_413++;
            addStringBlock(render->block, "HTTP/1.0 413 Request Entity Too Large\r\n", 39);
            break;
        case 507:
            render->stats->http_507++;
            addStringBlock(render->block, "HTTP/1.0 507 Insufficient Storage\r\n", 35);
            break;
        case 200:
        default:
            render->stats->http_200++;
            addStringBlock(render->block, "HTTP/1.1 200 OK\r\n", 17);
            break;
    }

    if (render->code != 200 && render->code != 101) {
        body = initBlock();
        addFormatStringBlock(body, render->size + 1000,
                             "d"
                             "14:failure reason"
                             "%zu:%s"
                             "e",
                             render->size,
                             render->message
        );
        render->size = body->size;
    }

    // End of headers
    if (render->websocketKey == NULL) {
        if (render->canKeepAlive) {
            addStringBlock(render->block, "Connection: Keep-Alive\r\n", 24);
            addFormatStringBlock(render->block, 100, "Keep-Alive: timeout=%u, max=1000\r\n", render->socketTimeout);
        } else {
            addStringBlock(render->block, "Connection: Close\r\n", 19);
        }
    }

    if (render->charset == NULL && render->contentType == NULL) {
        addFormatStringBlock(render->block, 1000, "Content-Type: text/plain\r\n");
    } else if (render->charset == NULL && render->contentType != NULL) {
        addFormatStringBlock(render->block, 1000, "Content-Type: %s\r\n", render->contentType);
    } else if (render->charset != NULL && render->contentType == NULL) {
        addFormatStringBlock(render->block, 1000, "Content-Type: text/plain; charset=%s\r\n", render->charset);
    } else if (render->charset != NULL && render->contentType != NULL) {
        addFormatStringBlock(render->block, 1000, "Content-Type: %s; charset=%s\r\n", render->contentType,
                             render->charset);
    }

    addFormatStringBlock(render->block, 1000, "Content-Length: %zu\r\n"
                                              "Server: github.com/truekenny/etracker\r\n"
                                              "\r\n",
                         render->size
    );

    // Body
    if (render->code == 200 || render->code == 101) {
        addStringBlock(render->block, render->message, render->size);
    } else {
        addStringBlock(render->block, body->data, body->size);
        freeBlock(body);
    }
}

ssize_t send_(int socket, void *message, size_t size, struct stats *stats) {
    // send может принять не всё сразу и за раз доставить только часть данных
    ssize_t result;

    do {
        result = send(socket, message, size, MSG_DONTWAIT | MSG_NOSIGNAL);

        if (result == -1) {
            incErrno(stats->send_errno);

            stats->send_failed++;
            // printf("send errno:%d message:%s\n", errno, strerror(errno));

            // Если ошибка не errno:35 message:Resource temporarily unavailable
            // то повторять не стоит
            if (errno != EAGAIN) break;
        } else
            stats->send_pass++;

        if (result >= 0) {
            stats->sent_bytes += result;

            // Отправить надо меньше на result байт
            size -= result;
            // Указатель надо сместить на result байт
            message += result;
        }
    } while (size != 0);

    return result;
}
