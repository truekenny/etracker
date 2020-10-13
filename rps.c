#include <time.h>
#include "rps.h"

// Замеряемый период
#define RPS_PERIOD_S 1

#define RPS_TIME_EVEN 0
#define RPS_TIME_ODD  1

void updateRps(struct rps *rps, unsigned char protocol) {
    if (time(NULL) / RPS_PERIOD_S % 2) {
        // odd minute
        if (rps->status[protocol] == RPS_TIME_EVEN) {
            rps->status[protocol] = RPS_TIME_ODD;
            rps->odd[protocol] = 0;
        } else {
            rps->odd[protocol]++;
        }
    } else {
        // even minute
        if (rps->status[protocol] == RPS_TIME_ODD) {
            rps->status[protocol] = RPS_TIME_EVEN;
            rps->even[protocol] = 0;
        } else {
            rps->even[protocol]++;
        }
    }
}

float getRps(struct rps *rps, unsigned char protocol) {
    float rpm = (float) ((rps->status[protocol] == RPS_TIME_ODD) ? rps->even[protocol] : rps->odd[protocol]);

    return rpm / RPS_PERIOD_S;
}
