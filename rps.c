#include <time.h>
#include "rps.h"

#define ONE_MINUTE_SECONDS 60

void updateRps(struct rps *rps) {
    if (time(NULL) / ONE_MINUTE_SECONDS % 2) {
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
    float rpm = (float) ((time(NULL) / ONE_MINUTE_SECONDS % 2) ? rps->even : rps->odd);

    return rpm / ONE_MINUTE_SECONDS;
}
