#include <time.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include "rps.h"
#include "thread.h"

// Замеряемый период
#define RPS_PERIOD_S 1

#define RPS_TIME_EVEN 0
#define RPS_TIME_ODD  1

unsigned char getRpsIndex(unsigned char protocol, unsigned char ipVersion);

void updateRps(struct rps *rps, unsigned char protocol, unsigned char ipVersion) {
    unsigned char index = getRpsIndex(protocol, ipVersion);

    if (rps->status == RPS_TIME_ODD) {
        rps->odd[index]++;
        if (rps->odd[index] > rps->max[index]) {
            rps->max[index] = rps->odd[index];
        }
    } else {
        rps->even[index]++;
        if (rps->even[index] > rps->max[index]) {
            rps->max[index] = rps->even[index];
        }
    }
}

float getRps(struct rps *rps, unsigned char protocol, unsigned char ipVersion) {
    unsigned char index = getRpsIndex(protocol, ipVersion);

    float rpm = (float) ((rps->status == RPS_TIME_ODD) ? rps->even[index] : rps->odd[index]);

    return rpm / RPS_PERIOD_S;
}

unsigned char getRpsIndex(unsigned char protocol, unsigned char ipVersion) {
    return protocol + ipVersion;
}

void resetMaxRps(struct rps *rps) {
    for (int index = 0; index < RPS_DIFFERENT_VALUES; ++index) {
        rps->max[index] = 0;
    }
}

void *rpsStatusHandler(struct rps *rps) {
    pthreadSetName(pthread_self(), "RPS Status");

    while (1) {
        if (rps->status == RPS_TIME_EVEN) {
            for (int index = 0; index < RPS_DIFFERENT_VALUES; ++index) {
                rps->odd[index] = 0;
            }
            rps->status = RPS_TIME_ODD;
        } else {
            for (int index = 0; index < RPS_DIFFERENT_VALUES; ++index) {
                rps->even[index] = 0;
            }
            rps->status = RPS_TIME_EVEN;
        }

        sleep(RPS_PERIOD_S);
        // Чтобы нормально работала подсветка кода в IDE
        if (rand() % 2 == 3) break;
    }

    return 0;
}

void runRpsStatusThread(struct rps *rps) {
    pthread_t tid;
    pthread_create(&tid, NULL, (void *(*)(void *)) rpsStatusHandler, rps);
}
