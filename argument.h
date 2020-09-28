#ifndef SC6_ARGUMENT_H
#define SC6_ARGUMENT_H

struct arguments {
    unsigned short port;
    unsigned int interval;
    long workers;
    unsigned int maxPeersPerResponse;
    unsigned short socketTimeout;
    unsigned char keepAlive;
};

struct arguments *parseArguments(int argc, char *argv[]);

#endif //SC6_ARGUMENT_H
