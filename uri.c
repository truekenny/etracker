#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include "uri.h"

#define URI_STATUS_PATH  0
#define URI_STATUS_PARAM 1
#define URI_STATUS_VALUE 2

void getParam(struct query *query, struct block *block, char *param, char *value);

/**
 * Разбор URI строки
 * @param message
 */
void parseUri(struct query *query, struct block *block, char *message) {
    char param[URI_PARAM_VALUE_LENGTH + 1] = {0};
    char value[URI_PARAM_VALUE_LENGTH + 1] = {0};
    int status = URI_STATUS_PATH;
    size_t len;
    short percent = 0;
    char percentChars[3] = {0};

    for (unsigned long i = strlen("GET "); i < strlen(message); i++) {
        char current = message[i];
        if (current == ' ') {
            getParam(query, block, &param[0], &value[0]);
            break;
        }

        if (status == URI_STATUS_PATH) {
            if (current == '?') {
                status = URI_STATUS_PARAM;

                continue;
            }
            if ((len = strlen(query->path)) < URI_PATH_LENGTH) {
                if(len != 0 || current != '/')
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
            getParam(query, block, &param[0], &value[0]);

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
