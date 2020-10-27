#ifndef SC6_RPS_H
#define SC6_RPS_H

#include <stdatomic.h>

#define RPS_PROTOCOL_TCP 0
#define RPS_PROTOCOL_UDP 1
#define RPS_VERSION_IPV4 0
#define RPS_VERSION_IPV6 2

#define RPS_DIFFERENT_VALUES 4

struct rps {
    atomic_uint even[RPS_DIFFERENT_VALUES]; // Четный
    atomic_uint odd[RPS_DIFFERENT_VALUES];  // Нечетный
    atomic_uint max[RPS_DIFFERENT_VALUES];  // Максимальные зарегистрированные значения
    atomic_uchar status; // Единый статус для всех значений
};

void updateRps(struct rps *rps, unsigned char protocol, unsigned char ipVersion);

float getRps(struct rps *rps, unsigned char protocol, unsigned char ipVersion);

void resetMaxRps(struct rps *rps);

void runRpsStatusThread(struct rps *rps);

#endif //SC6_RPS_H
