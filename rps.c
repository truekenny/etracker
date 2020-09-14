#include <time.h>
#include "rps.h"

// Замеряемый период
#define PERIOD 1

void updateRps(struct rps *rps) {
    if (time(NULL) / PERIOD % 2) {
        // odd minute
        if (rps->status == TIME_EVEN) {
            rps->status = TIME_ODD;
            rps->odd = 0;
        } else {
            rps->odd++;
        }
    } else {
        // even minute
        if (rps->status == TIME_ODD) {
            rps->status = TIME_EVEN;
            rps->even = 0;
        } else {
            rps->even++;
        }
    }
}

float getRps(struct rps *rps) {
    //float rpm = (float) ((time(NULL) / PERIOD % 2) ? rps->even : rps->odd);
    float rpm = (float) ((rps->status == TIME_ODD) ? rps->even : rps->odd);

    return rpm / PERIOD;
}
