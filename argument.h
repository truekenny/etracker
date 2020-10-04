#ifndef SC6_ARGUMENT_H
#define SC6_ARGUMENT_H

#include <stdatomic.h>

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
};

struct arguments *parseArguments(int argc, char *argv[]);

#endif //SC6_ARGUMENT_H
