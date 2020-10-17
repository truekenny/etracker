#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "geoip.h"
#include "exit_code.h"
#include "alloc.h"

#define GEOIP_FILE_LINES 2912124
#define GEOIP_POSITION_START_IP_GEOIP 1
#define GEOIP_POSITION_END_IP_GEOIP   2
#define GEOIP_POSITION_LAT_GEOIP      7
#define GEOIP_POSITION_LON_GEOIP      8

#define GEOIP_FILE "IP2LOCATION-LITE-DB5.CSV"

#define GEOIP_MAX_ITERATION 50

struct geoip *initGeoip(unsigned char noLocations) {
    struct geoip *geoip = c_calloc(noLocations ? 1 : GEOIP_FILE_LINES, sizeof(struct geoip));

    if (geoip == NULL)
        exitPrint(EXIT_CODE_GEOIP_MEMORY, __FILE__, EXIT_CODE_PRINT_ERROR_YES);

    return geoip;
}

void freeGeoip(struct geoip *geoip) {
    c_free(geoip);
}

void loadGeoip(struct geoip *geoip) {
    FILE *file;
    char *line = NULL;
    size_t length = 0;

    file = fopen(GEOIP_FILE, "r");
    if (file == NULL) {
        file = fopen("../" GEOIP_FILE, "r");

        if (file == NULL) {

            printf(GEOIP_FILE " not found (wget http://tace.ru/IP2LOCATION-LITE-DB5.CSV)\n");
            return;
        }
    }

    printf("Loading locations…\n");

    unsigned int startIp = 0;

    unsigned int lineNumber = 0;
    while (getline(&line, &length, file) != -1) {
        // printf("Retrieved line of length %zu: %s\n", readBytes, line);

        char *cell = strtok(line, ",\"");
        unsigned char position = 0;
        while (cell != NULL) {
            position++;
            // printf("+%s\n", cell);
            if (position == GEOIP_POSITION_START_IP_GEOIP) {
                geoip[lineNumber].startIp = atoi(cell);

                if (startIp > geoip[lineNumber].startIp)
                    exitPrint(EXIT_CODE_GEOIP_WRONG_DATA, __FILE__, EXIT_CODE_PRINT_ERROR_NO);

                startIp = geoip[lineNumber].startIp;
            } else if (position == GEOIP_POSITION_END_IP_GEOIP) {
                geoip[lineNumber].endIp = atoi(cell);
            } else if (position == GEOIP_POSITION_LAT_GEOIP) {
                geoip[lineNumber].lat = atof(cell);
            } else if (position == GEOIP_POSITION_LON_GEOIP) {
                geoip[lineNumber].lon = atof(cell);
            }

            cell = strtok(NULL, ",\"");
        }

        lineNumber++;
    }

    fclose(file);
    if (line)
        free(line);
    printf("Loading locations finished.\n");
}

struct geoip *findGeoip(struct geoip *geoip, unsigned int ip) {
    if (geoip->endIp == 0)
        return geoip;

    unsigned long long delimiter = 2;
    unsigned int index = GEOIP_FILE_LINES / delimiter;
    unsigned int newIndex;
    static atomic_int max_iterations = 0;
    int current_iterations = 0;

    struct geoip *middle = &geoip[index];

    while (middle->startIp > ip || middle->endIp < ip) {
        delimiter *= 2;
        newIndex = index;

        if (middle->startIp > ip) {
            newIndex -= GEOIP_FILE_LINES / delimiter;
            if (index != newIndex)
                index = newIndex;
            else {
                index = newIndex - 1;
                // printf("-\n");
            }
        } else {
            newIndex += GEOIP_FILE_LINES / delimiter;
            if (index != newIndex)
                index = newIndex;
            else {
                index = newIndex + 1;
                // printf("+\n");
            }
        }

        middle = &geoip[index];
        current_iterations++;

        if (current_iterations > GEOIP_MAX_ITERATION) {
            printf("Too many iteration %d for ip: %u\n", current_iterations, ip);

            break;
        }
    }

    if (current_iterations > max_iterations) {
        max_iterations = current_iterations;
        printf("ip: %u, iterations: %d\n", ip, current_iterations);
    }

    return middle;
}
