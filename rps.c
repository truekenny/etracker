#include <time.h>
#include "rps.h"

// Замеряемый период
#define RPS_PERIOD_S 1

#define RPS_TIME_EVEN 0
#define RPS_TIME_ODD  1

unsigned char getRpsIndex(unsigned char protocol, unsigned char ipVersion);

void updateRps(struct rps *rps, unsigned char protocol, unsigned char ipVersion) {
    unsigned char index = getRpsIndex(protocol, ipVersion);

    if (time(NULL) / RPS_PERIOD_S % 2) {
        // odd minute
        if (rps->status[index] == RPS_TIME_EVEN) {
            rps->status[index] = RPS_TIME_ODD;
            rps->odd[index] = 0;
        } else {
            rps->odd[index]++;

            if (rps->odd[index] > rps->max[index]) {
                rps->max[index] = rps->odd[index];
            }
        }
    } else {
        // even minute
        if (rps->status[index] == RPS_TIME_ODD) {
            rps->status[index] = RPS_TIME_EVEN;
            rps->even[index] = 0;
        } else {
            rps->even[index]++;

            if (rps->even[index] > rps->max[index]) {
                rps->max[index] = rps->even[index];
            }
        }
    }
}

float getRps(struct rps *rps, unsigned char protocol, unsigned char ipVersion) {
    unsigned char index = getRpsIndex(protocol, ipVersion);

    float rpm = (float) ((rps->status[index] == RPS_TIME_ODD) ? rps->even[index] : rps->odd[index]);

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
