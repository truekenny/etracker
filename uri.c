#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "uri.h"

#define DEBUG 1
#define URI_PATH 0
#define QUERY_PARAM 1
#define QUERY_VALUE 2
#define PATH_LENGTH 50
#define FIRST_LINE_LENGTH 1000

void getParam(struct query *query, char *param, char *value);

/**
 * Разбор URI строки
 * @param message
 */
void parseUri(struct query *query, char *message) {
    DEBUG && printf("Uri:\n");

    if (DEBUG) {
        char firstLine[FIRST_LINE_LENGTH] = {0};

        int len = strchr(message, '\r') - message;
        if (len < FIRST_LINE_LENGTH) {
            memcpy(firstLine, message, len);
            printf("%s\n", firstLine);
        } else
            printf("Len of first line = %d\n", len);
    }

    char path[PATH_LENGTH + 1] = {0},
            param[PARAM_VALUE_LENGTH + 1] = {0},
            value[PARAM_VALUE_LENGTH + 1] = {0};
    int status = URI_PATH;
    size_t len;


    for (int i = strlen("GET "); i < strlen(message); i++) {
        char current = message[i];
        if (current == ' ') {
            DEBUG && printf("Param:%s=Value:%s, %lu %lu %d\n", param, value, strlen(param), strlen(value),
                            strcmp(param, "123456"));
            getParam(query, &param[0], &value[0]);
            break;
        }
        DEBUG && printf("%d=%c\n", i, current);
        if (status == URI_PATH) {
            if (current == '?') {
                status = QUERY_PARAM;
                DEBUG && printf("Path:%s, %lu\n", path, strlen(path));
                DEBUG && printf("Status=PARAM\n");
                continue;
            }
            if ((len = strlen(path)) < PATH_LENGTH) {
                path[len] = current;
            }
            continue;
        }
        if (current == '=') {
            memset(&value, 0, PARAM_VALUE_LENGTH);
            status = QUERY_VALUE;
            continue;
        }
        if (current == '&') {
            DEBUG && printf("Param:%s=Value:%s, %lu %lu %d\n", param, value, strlen(param), strlen(value),
                            strcmp(param, "123456"));
            getParam(query, &param[0], &value[0]);

            memset(&param, 0, PARAM_VALUE_LENGTH);
            status = QUERY_PARAM;

            continue;
        }
        if (status == QUERY_PARAM && (len = strlen(param)) < PARAM_VALUE_LENGTH) {
            param[len] = current;
        }
        if (status == QUERY_VALUE && (len = strlen(value)) < PARAM_VALUE_LENGTH) {
            value[len] = current;
        }
    }
    DEBUG && printf(".\n");
}

/**
 * Заполняет структуру параметрами запроса
 * @param query
 * @param param
 * @param value
 */
void getParam(struct query *query, char *param, char *value) {
    if (!strcmp(param, "info_hash")) {
        memcpy(query->info_hash, value, PARAM_VALUE_LENGTH);
    } else if (!strcmp(param, "event")) {
        if (!strcmp(value, EVENT_STRING_COMPLETED)) {
            query->event = EVENT_ID_COMPLETED;
        } else if (!strcmp(value, EVENT_STRING_STARTED)) {
            query->event = EVENT_ID_STARTED;
        } else if (!strcmp(value, EVENT_STRING_STOPPED)) {
            query->event = EVENT_ID_STOPPED;
        }
    } else if (!strcmp(param, "port")) {
        query->port = atoi(value);
    } else if (!strcmp(param, "peer_id")) {
        memcpy(query->peer_id, value, PARAM_VALUE_LENGTH);
    } else if (!strcmp(param, "compact")) {
        query->compact = 1;
    } else if (!strcmp(param, "no_peer_id")) {
        query->no_peer_id = 1;
    }
}
