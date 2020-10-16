#ifndef SC6_RPS_H
#define SC6_RPS_H

#include <stdatomic.h>

#define RPS_PROTOCOL_TCP 0
#define RPS_PROTOCOL_UDP 1
#define RPS_VERSION_IPV4 0
#define RPS_VERSION_IPV6 2

struct rps {
    atomic_uint even[4]; // Четный
    atomic_uint odd[4];  // Нечетный
    atomic_uchar status[4];
};

void updateRps(struct rps *rps, unsigned char protocol, unsigned char ipVersion);

float getRps(struct rps *rps, unsigned char protocol, unsigned char ipVersion);

#endif //SC6_RPS_H
