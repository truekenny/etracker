#ifndef SC6_RPS_H
#define SC6_RPS_H

#include <stdatomic.h>

#define RPS_TCP 0
#define RPS_UDP 1

#define TIME_EVEN 0
#define TIME_ODD 1

struct rps {
    atomic_uint even[2]; // Четный
    atomic_uint odd[2]; // Нечетный
    atomic_uchar status[2];
};

void updateRps(struct rps *rps, unsigned char protocol);

float getRps(struct rps *rps, unsigned char protocol);

#endif //SC6_RPS_H
