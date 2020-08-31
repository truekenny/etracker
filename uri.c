#include <stdio.h>
#include <string.h>
#include "uri.h"

#define DEBUG 0
#define PATH 0
#define PARAM 1
#define VALUE 2
#define PATH_SIZE 50
#define PARAM_VALUE_SIZE 20

/**
 * Разбор URI строки
 * @param message
 */
void parseUri(char *message) {
    DEBUG && printf("Uri:\n");

    if (DEBUG) {
        char firstLine[200] = {0};
        memcpy(firstLine, message, strchr(message, '\r') - message);
        printf("%s\n", firstLine);
    }

    char path[PATH_SIZE + 1] = {0},
            param[PARAM_VALUE_SIZE + 1] = {0},
            value[PARAM_VALUE_SIZE + 1] = {0};
    int status = PATH;
    size_t len;

    for (int i = strlen("GET "); i < strlen(message); i++) {
        char current = message[i];
        if (current == ' ') {
            DEBUG && printf("Param:%s=Value:%s, %lu %lu %d\n", param, value, strlen(param), strlen(value),
                            strcmp(param, "123456"));
            break;
        }
        DEBUG && printf("%d=%c\n", i, current);
        if (status == PATH) {
            if (current == '?') {
                status = PARAM;
                DEBUG && printf("Path:%s, %lu\n", path, strlen(path));
                DEBUG && printf("Status=PARAM\n");
                continue;
            }
            if ((len = strlen(path)) < PATH_SIZE) {
                path[len] = current;
            }
            continue;
        }
        if (current == '=') {
            memset(&value, 0, PARAM_VALUE_SIZE);
            status = VALUE;
            continue;
        }
        if (current == '&') {
            DEBUG && printf("Param:%s=Value:%s, %lu %lu %d\n", param, value, strlen(param), strlen(value),
                            strcmp(param, "123456"));

            memset(&param, 0, PARAM_VALUE_SIZE);
            status = PARAM;

            continue;
        }
        if (status == PARAM && (len = strlen(param)) < PARAM_VALUE_SIZE) {
            param[len] = current;
        }
        if (status == VALUE && (len = strlen(value)) < PARAM_VALUE_SIZE) {
            value[len] = current;
        }
    }
    DEBUG && printf(".\n");
}
