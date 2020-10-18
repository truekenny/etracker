#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include "uri.h"
#include "string.h"
#include "socket.h"

#define URI_STATUS_PATH  0
#define URI_STATUS_PARAM 1
#define URI_STATUS_VALUE 2

#define URI_STATUS_RESET 0
#define URI_STATUS_NICE  1

void getParam(struct query *query, struct block *block, char *param, char *value);

void parseXForwardedFor(struct query *query, char *message, struct block *forwardedForBlock);

/**
 * Разбор URI строки
 * @param message
 */
void parseUri(struct query *query, struct block *hashesBlock, struct block *forwardedForBlock, char *message) {
    char param[URI_PARAM_VALUE_LENGTH + 1] = {0};
    char value[URI_PARAM_VALUE_LENGTH + 1] = {0};
    int status = URI_STATUS_PATH;
    size_t len;
    short percent = 0;
    char percentChars[3] = {0};

    for (unsigned long i = strlen("GET "); i < strlen(message); i++) {
        char current = message[i];
        if (current == ' ') {
            getParam(query, hashesBlock, &param[0], &value[0]);
            break;
        }

        if (status == URI_STATUS_PATH) {
            if (current == '?') {
                status = URI_STATUS_PARAM;

                continue;
            }
            if ((len = strlen(query->path)) < URI_PATH_LENGTH) {
                if (len != 0 || current != '/')
                    query->path[len] = current;
            }
            continue;
        }
        if (current == '=') {
            memset(&value, 0, URI_PARAM_VALUE_LENGTH);
            status = URI_STATUS_VALUE;
            len = 0;
            continue;
        }
        if (current == '&') {
            getParam(query, hashesBlock, &param[0], &value[0]);

            memset(&param, 0, URI_PARAM_VALUE_LENGTH);
            status = URI_STATUS_PARAM;

            continue;
        }
        if (status == URI_STATUS_PARAM && (len = strlen(param)) < URI_PARAM_VALUE_LENGTH) {
            param[len] = current;
        }
        if (status == URI_STATUS_VALUE && len < URI_PARAM_VALUE_LENGTH) {
            if (current == '%') {
                percent = 1;
                continue;
            }
            if (percent == 1) {
                percentChars[0] = current; // '4';
                percent = 2;
                continue;
            }
            if (percent == 2) {
                percent = 0;
                percentChars[1] = current; // '1';

                // 0x41 => 'A'
                current = (char) strtol(percentChars, NULL, 16);
            }

            value[len] = current;
            len++;
        }
    }

    parseXForwardedFor(query, message, forwardedForBlock);
}

void parseXForwardedFor(struct query *query, char *message, struct block *forwardedForBlock) {
    // Этот разбор не заинтересован в этом заголовке
    if (forwardedForBlock == NULL)
        return;

    // Конфигурация не требует разбора заголовка
    if (query->xForwardedFor == NULL)
        return;

    char *endOfFirstLine = strstr(message, "\r\n");
    // Не найден конец первой линии
    if (endOfFirstLine == NULL)
        return;

    char *xForwardedFor = strstr(message, query->xForwardedFor);
    // Заголовок не найден
    if (xForwardedFor == NULL)
        return;

    // Заголовок в строке запроса
    if (xForwardedFor < endOfFirstLine)
        return;

    char *endOfHeaders = strstr(message, "\r\n\r\n");
    // Заголовок за пределами обрасти заголовков
    if (endOfHeaders < xForwardedFor)
        return;

    char *xForwardedForEndName = strstr(xForwardedFor, ":");
    // Не найден разделитель имени и значения заголовка
    if (xForwardedForEndName == NULL)
        return;
    xForwardedForEndName += 1; // + ':'

    char *xForwardedForEndValue = strstr(xForwardedFor, "\r\n");
    // Не найден конец заголовка
    if (xForwardedForEndValue == NULL)
        return;
    // Заголовок не содержит разделителя «:»
    if (xForwardedForEndValue < xForwardedForEndName)
        return;


    forwardedForBlock = resetBlock(forwardedForBlock);

    int status = URI_STATUS_RESET;
    for (long index = 0; index < xForwardedForEndValue - xForwardedForEndName; ++index) {
        char currentChar = xForwardedForEndName[index];

        if (currentChar == ' ' || currentChar == ',')
            status = URI_STATUS_RESET;
        else {
            if (status == URI_STATUS_RESET)
                forwardedForBlock = resetBlock(forwardedForBlock);

            addStringBlock(forwardedForBlock, &currentChar, 1);

            status = URI_STATUS_NICE;
        }
    }

    // В структуре forwardedForBlock ip в строковом формате

    unsigned char ip[17] = {};

    if (strstr(forwardedForBlock->data, ":") == NULL) {
        // ipv4
        if (inet_pton(AF_INET, forwardedForBlock->data, ip) < 0) {
            resetBlock(forwardedForBlock);

        } else {
            resetBlock(forwardedForBlock);
            // inet_pton может вернуть 4 нулевых байта
            if (*(int *) ip != 0) {
                // Приведение заголовка к ipv6 виду
                addStringBlock(forwardedForBlock,
                               "\x00\x00\x00\x00" "\x00\x00\x00\x00" "\x00\x00\xff\xff", 12);
                addStringBlock(forwardedForBlock, ip, 4);

                memcpy(&query->ip, forwardedForBlock->data, 16);
                query->ipVersion = SOCKET_VERSION_IPV4_BIT;
            }
        }
    } else {
        // ipv6
        if (inet_pton(AF_INET6, forwardedForBlock->data, ip) < 0) {
            resetBlock(forwardedForBlock);
        } else {
            resetBlock(forwardedForBlock);
            addStringBlock(forwardedForBlock, ip, 16);

            memcpy(&query->ip, forwardedForBlock->data, 16);
            query->ipVersion = SOCKET_VERSION_IPV6_BIT;
        }
    };
}

/**
 * Заполняет структуру параметрами запроса
 * @param query
 * @param param
 * @param value
 */
void getParam(struct query *query, struct block *block, char *param, char *value) {
    if (!strcmp(param, "info_hash")) {
        memcpy(query->info_hash, value, URI_PARAM_VALUE_LENGTH);
        query->has_info_hash = 1;

        // Random peers
        if (URI_RANDOM_DATA_INFO_HASH) {
            query->info_hash[0] = (rand() % 256);
            query->info_hash[1] = (rand() % 256);
            query->info_hash[2] = (rand() % 256);
            query->info_hash[3] = (rand() % 256);
        }

        if (block != NULL) {
            addStringBlock(block, query->info_hash, URI_PARAM_VALUE_LENGTH);
        }

    } else if (!strcmp(param, "event")) {
        if (!strcmp(value, URI_EVENT_STRING_COMPLETED)) {
            query->event = URI_EVENT_ID_COMPLETED;
        } else if (!strcmp(value, URI_EVENT_STRING_STARTED)) {
            query->event = URI_EVENT_ID_STARTED;
        } else if (!strcmp(value, URI_EVENT_STRING_STOPPED)) {
            query->event = URI_EVENT_ID_STOPPED;
        } else if (!strcmp(value, URI_EVENT_STRING_PAUSED)) {
            query->event = URI_EVENT_ID_PAUSED;
        } else {
            query->event = URI_EVENT_ID_CONTINUE;
        }
    } else if (!strcmp(param, "port")) {
        query->port = htons(atoi(value));
    } else if (!strcmp(param, "peer_id")) {
        memcpy(query->peer_id, value, URI_PARAM_VALUE_LENGTH);

        // Random peers
        if (URI_RANDOM_DATA_PEER_ID) {
            query->peer_id[URI_PARAM_VALUE_LENGTH - 1] = (rand() % 256);
            query->peer_id[URI_PARAM_VALUE_LENGTH - 2] = (rand() % 256);
            query->peer_id[URI_PARAM_VALUE_LENGTH - 3] = (rand() % 256);
            query->peer_id[URI_PARAM_VALUE_LENGTH - 4] = (rand() % 256);
        }

    } else if (!strcmp(param, "compact")) {
        query->compact = atoi(value);
    } else if (!strcmp(param, "no_peer_id")) {
        query->no_peer_id = 1;
    } else if (!strcmp(param, "numwant")) {
        query->numwant = atoi(value);
    } else if (!strcmp(param, "interval")) {
        query->interval = atoi(value);
    } else if (!strcmp(param, "max_peers_response")) {
        query->max_peers_per_response = atoi(value);
    } else if (!strcmp(param, "socket_timeout")) {
        query->socket_timeout = atoi(value);
    } else if (!strcmp(param, "keep_alive")) {
        query->keep_alive = atoi(value);
    }
}
