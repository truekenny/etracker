#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "geoip.h"
#include "exit_code.h"
#include "alloc.h"

#define GEOIP_LINES 2912124
#define POSITION_START_IP_GEOIP 1
#define POSITION_END_IP_GEOIP 2
#define POSITION_LAT_GEOIP 7
#define POSITION_LON_GEOIP 8

#define GEOIP_FILE "IP2LOCATION-LITE-DB5.CSV"

struct geoip *initGeoip() {
    struct geoip *geoip = c_calloc(GEOIP_LINES, sizeof(struct geoip));

    if (geoip == NULL)
        exitPrint(EXIT_GEOIP_MEMORY, __FILE__, PRINT_ERROR_YES);

    return geoip;
}

void freeGeoip(struct geoip *geoip) {
    c_free(geoip);
}

void loadGeoip(struct geoip *geoip) {
    FILE *file;
    char *line = NULL;
    size_t length = 0;
    ssize_t readBytes;

    file = fopen(GEOIP_FILE, "r");
    if (file == NULL) {
        file = fopen("../" GEOIP_FILE, "r");

        if (file == NULL) {

            printf(GEOIP_FILE " not found (wget http://tace.ru/IP2LOCATION-LITE-DB5.CSV)\n");
            return;
        }
    }

    printf("Loading locations\n");

    unsigned int startIp = 0;

    unsigned int lineNumber = 0;
    while ((readBytes = getline(&line, &length, file)) != -1) {
        // printf("Retrieved line of length %zu: %s\n", readBytes, line);

        char *cell = strtok(line, ",\"");
        unsigned char position = 0;
        while (cell != NULL) {
            position++;
            // printf("+%s\n", cell);
            if (position == POSITION_START_IP_GEOIP) {
                geoip[lineNumber].startIp = atoi(cell);

                if (startIp > geoip[lineNumber].startIp)
                    exitPrint(EXIT_GEOIP_WRONG_DATA, __FILE__, PRINT_ERROR_NO);

                startIp = geoip[lineNumber].startIp;
            } else if (position == POSITION_END_IP_GEOIP) {
                geoip[lineNumber].endIp = atoi(cell);
            } else if (position == POSITION_LAT_GEOIP) {
                geoip[lineNumber].lat = atoi(cell);
            } else if (position == POSITION_LON_GEOIP) {
                geoip[lineNumber].lon = atoi(cell);
            }

            cell = strtok(NULL, ",\"");
        }

        lineNumber++;
    }

    fclose(file);
    if (line)
        free(line);
    printf("Loading complete\n");
}

struct geoip *findGeoip(struct geoip *geoip, unsigned int ip) {
    if (geoip->endIp == 0)
        return geoip;

    unsigned long long delimiter = 2;
    unsigned int index = GEOIP_LINES / delimiter;
    unsigned int newIndex;

    struct geoip *middle = &geoip[index];

    while (middle->startIp > ip || middle->endIp < ip) {
        delimiter *= 2;
        newIndex = index;

        if (middle->startIp > ip) {
            newIndex -= GEOIP_LINES / delimiter;
            if (index != newIndex)
                index = newIndex;
            else {
                index = newIndex - 1;
                // printf("-\n");
            }
        } else {
            newIndex += GEOIP_LINES / delimiter;
            if (index != newIndex)
                index = newIndex;
            else {
                index = newIndex + 1;
                // printf("+\n");
            }
        }

        middle = &geoip[index];
    }

    return middle;
}
