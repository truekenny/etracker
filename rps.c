#include <time.h>
#include "rps.h"

// Замеряемый период
#define PERIOD 1

void updateRps(struct rps *rps, unsigned char protocol) {
    if (time(NULL) / PERIOD % 2) {
        // odd minute
        if (rps->status[protocol] == TIME_EVEN) {
            rps->status[protocol] = TIME_ODD;
            rps->odd[protocol] = 0;
        } else {
            rps->odd[protocol]++;
        }
    } else {
        // even minute
        if (rps->status[protocol] == TIME_ODD) {
            rps->status[protocol] = TIME_EVEN;
            rps->even[protocol] = 0;
        } else {
            rps->even[protocol]++;
        }
    }
}

float getRps(struct rps *rps, unsigned char protocol) {
    float rpm = (float) ((rps->status[protocol] == TIME_ODD) ? rps->even[protocol] : rps->odd[protocol]);

    return rpm / PERIOD;
}
