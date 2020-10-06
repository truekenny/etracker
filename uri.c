#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include "uri.h"

#define URI_PATH 0
#define QUERY_PARAM 1
#define QUERY_VALUE 2

void getParam(struct query *query, struct block *block, char *param, char *value);

/**
 * Разбор URI строки
 * @param message
 */
void parseUri(struct query *query, struct block *block, char *message) {
    char param[PARAM_VALUE_LENGTH + 1] = {0};
    char value[PARAM_VALUE_LENGTH + 1] = {0};
    int status = URI_PATH;
    size_t len;
    short percent = 0;
    char percentChars[3] = {0};

    for (unsigned long i = strlen("GET "); i < strlen(message); i++) {
        char current = message[i];
        if (current == ' ') {
            getParam(query, block, &param[0], &value[0]);
            break;
        }

        if (status == URI_PATH) {
            if (current == '?') {
                status = QUERY_PARAM;

                continue;
            }
            if ((len = strlen(query->path)) < PATH_LENGTH) {
                if(len != 0 || current != '/')
                    query->path[len] = current;
            }
            continue;
        }
        if (current == '=') {
            memset(&value, 0, PARAM_VALUE_LENGTH);
            status = QUERY_VALUE;
            len = 0;
            continue;
        }
        if (current == '&') {
            getParam(query, block, &param[0], &value[0]);

            memset(&param, 0, PARAM_VALUE_LENGTH);
            status = QUERY_PARAM;

            continue;
        }
        if (status == QUERY_PARAM && (len = strlen(param)) < PARAM_VALUE_LENGTH) {
            param[len] = current;
        }
        if (status == QUERY_VALUE && len < PARAM_VALUE_LENGTH) {
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
        memcpy(query->info_hash, value, PARAM_VALUE_LENGTH);
        query->has_info_hash = 1;

        // Random peers
        if (RANDOM_DATA_INFO_HASH) {
            query->info_hash[0] = (rand() % 256);
            query->info_hash[1] = (rand() % 256);
            query->info_hash[2] = (rand() % 256);
            query->info_hash[3] = (rand() % 256);
        }

        if (block != NULL) {
            addStringBlock(block, query->info_hash, PARAM_VALUE_LENGTH);
        }

    } else if (!strcmp(param, "event")) {
        if (!strcmp(value, EVENT_STRING_COMPLETED)) {
            query->event = EVENT_ID_COMPLETED;
        } else if (!strcmp(value, EVENT_STRING_STARTED)) {
            query->event = EVENT_ID_STARTED;
        } else if (!strcmp(value, EVENT_STRING_STOPPED)) {
            query->event = EVENT_ID_STOPPED;
        } else if (!strcmp(value, EVENT_STRING_PAUSED)) {
            query->event = EVENT_ID_PAUSED;
        } else {
            query->event = EVENT_ID_CONTINUE;
        }
    } else if (!strcmp(param, "port")) {
        query->port = htons(atoi(value));
    } else if (!strcmp(param, "peer_id")) {
        memcpy(query->peer_id, value, PARAM_VALUE_LENGTH);

        // Random peers
        if (RANDOM_DATA_PEER_ID) {
            query->peer_id[PARAM_VALUE_LENGTH - 1] = (rand() % 256);
            query->peer_id[PARAM_VALUE_LENGTH - 2] = (rand() % 256);
            query->peer_id[PARAM_VALUE_LENGTH - 3] = (rand() % 256);
            query->peer_id[PARAM_VALUE_LENGTH - 4] = (rand() % 256);
        }

    } else if (!strcmp(param, "compact")) {
        query->compact = 1;
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
