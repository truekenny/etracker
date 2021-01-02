#ifndef SC6_ARGUMENT_H
#define SC6_ARGUMENT_H

#include <stdatomic.h>
#include <sys/resource.h>

struct arguments {
    unsigned short port;
    _Atomic (unsigned int) interval;
    long workers;
    unsigned int maxPeersPerResponse;
    unsigned short socketTimeout;
    unsigned char keepAlive;
    char *charset;
    unsigned int minInterval;
    unsigned int maxInterval;
    unsigned char noTcp;
    unsigned char noUdp;
    char *locale;
    rlim_t nofile;
    long long core;
    int failed;
    unsigned char noLocations;
    char *xForwardedFor;
    double maxLoadAvg;
};

struct arguments *parseArguments(int argc, char *argv[]);

#endif //SC6_ARGUMENT_H
