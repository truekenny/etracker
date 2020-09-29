#ifndef SC6_RPS_H
#define SC6_RPS_H

#include <stdatomic.h>

#define TIME_EVEN 0
#define TIME_ODD 1

struct rps {
    atomic_uint even; // Четный
    atomic_uint odd; // Нечетный
    atomic_uchar status;
};

void updateRps(struct rps *rps);

float getRps(struct rps *rps);

#endif //SC6_RPS_H
