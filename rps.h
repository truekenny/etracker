#ifndef SC6_RPS_H
#define SC6_RPS_H

#define TIME_EVEN 0
#define TIME_ODD 1

struct rps {
    unsigned even; // Четный
    unsigned odd; // Нечетный
    unsigned char status;
};

void updateRps(struct rps *rps);

float getRps(struct rps *rps);

#endif //SC6_RPS_H
