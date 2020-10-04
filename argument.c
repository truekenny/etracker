#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <unistd.h>
#include "argument.h"
#include "alloc.h"

#define UNKNOWN_NAME                0
#define PORT_NAME                   1
#define INTERVAL_NAME               2
#define WORKERS_NAME                3
#define MAX_PEERS_PER_RESPONSE_NAME 4
#define SOCKET_TIMEOUT_NAME         5
#define KEEP_ALIVE_NAME             6
#define HELP_NAME                   7
#define CHARSET_NAME                8
#define MIN_INTERVAL_NAME           9
#define MAX_INTERVAL_NAME          10

#define DEFAULT_PORT                3000
#define DEFAULT_INTERVAL            1799
#define DEFAULT_MAX_PEER_PER_RESULT 60
#define DEFAULT_SOCKET_TIMEOUT      3
#define DEFAULT_KEEP_ALIVE          0
#define DEFAULT_MIN_INTERVAL        239
#define DEFAULT_MAX_INTERVAL        1799

unsigned int getName(char *name);

void showHelp();

unsigned int minMaxUnsignedInt(unsigned int min, unsigned int value, unsigned int max) {
    if (value < min)
        value = min;

    if (value > max)
        value = max;

    return value;
}

struct arguments *parseArguments(int argc, char *argv[]) {
    // Первый аргумент не может быть числом – скорей всего используется старый формат
    if (argc > 1 && atoi(argv[1]))
        showHelp();

    struct arguments *arguments = c_calloc(1, sizeof(struct arguments));

    arguments->port = DEFAULT_PORT;
    arguments->interval = DEFAULT_INTERVAL;
    arguments->workers = sysconf(_SC_NPROCESSORS_ONLN);
    arguments->maxPeersPerResponse = DEFAULT_MAX_PEER_PER_RESULT;
    arguments->socketTimeout = DEFAULT_SOCKET_TIMEOUT;
    arguments->keepAlive = DEFAULT_KEEP_ALIVE;
    arguments->minInterval = DEFAULT_MIN_INTERVAL;
    arguments->maxInterval = DEFAULT_MAX_INTERVAL;

    for (int index = 0; index < argc; index++) {
        char *argumentName = argv[index];
        int argumentValue = (index == argc - 1) ? 0 : atoi(argv[index + 1]);

        switch (getName(argumentName)) {
            case PORT_NAME:
                arguments->port = argumentValue;
                break;
            case INTERVAL_NAME:
                arguments->interval = argumentValue;
                break;
            case WORKERS_NAME:
                arguments->workers = argumentValue;
                break;
            case MAX_PEERS_PER_RESPONSE_NAME:
                arguments->maxPeersPerResponse = argumentValue;
                break;
            case SOCKET_TIMEOUT_NAME:
                arguments->socketTimeout = argumentValue;
                break;
            case KEEP_ALIVE_NAME:
                arguments->keepAlive = 1;
                break;
            case HELP_NAME:
                showHelp();
                break;
            case CHARSET_NAME:
                arguments->charset = (index == argc - 1) ? NULL : argv[index + 1];
                break;
            case MIN_INTERVAL_NAME:
                arguments->minInterval = argumentValue;
                break;
            case MAX_INTERVAL_NAME:
                arguments->maxInterval = argumentValue;
                break;
        }
    }

    arguments->minInterval = minMaxUnsignedInt(DEFAULT_MIN_INTERVAL, arguments->minInterval, DEFAULT_MAX_INTERVAL);
    arguments->maxInterval = minMaxUnsignedInt(arguments->minInterval, arguments->maxInterval, DEFAULT_MAX_INTERVAL);

    if (!arguments->port
        || !arguments->interval
        || !arguments->workers
        || !arguments->maxPeersPerResponse
        || !arguments->socketTimeout
        || !arguments->minInterval
        || !arguments->maxInterval
            )
        showHelp();

    return arguments;
}

unsigned int getName(char *name) {
    if (!strcmp(name, "-p") || !strcmp(name, "--port")) {
        return PORT_NAME;
    } else if (!strcmp(name, "-i") || !strcmp(name, "--interval")) {
        return INTERVAL_NAME;
    } else if (!strcmp(name, "-w") || !strcmp(name, "--workers")) {
        return WORKERS_NAME;
    } else if (!strcmp(name, "-e") || !strcmp(name, "--peers")) {
        return MAX_PEERS_PER_RESPONSE_NAME;
    } else if (!strcmp(name, "-t") || !strcmp(name, "--timeout")) {
        return SOCKET_TIMEOUT_NAME;
    } else if (!strcmp(name, "-k") || !strcmp(name, "--keep")) {
        return KEEP_ALIVE_NAME;
    } else if (!strcmp(name, "-h") || !strcmp(name, "--help")) {
        return HELP_NAME;
    } else if (!strcmp(name, "--charset")) {
        return CHARSET_NAME;
    } else if (!strcmp(name, "--min-interval")) {
        return MIN_INTERVAL_NAME;
    } else if (!strcmp(name, "--max-interval")) {
        return MAX_INTERVAL_NAME;
    }

    return UNKNOWN_NAME;
}

void showHelp() {
    printf(
            "NAME\n"
            "     etracker -- Open-source BitTorrent tracker\n"
            "\n"
            "SYNOPSIS\n"
            "     etracker [-p port] [-i interval] [-w workers] [-e peers] [-t timeout] [-k] [-h] [--charset charset]\n"
            "\n"
            "DESCRIPTION\n"
            "     Run BitTorrent tracker with interfaces:\n"
            "     http://your_host:port/announce\n"
            "     udp://your_host:port/announce\n"
            "\n"
            "The following options are available:"
            "\n"
            "     -p      Port for http and udp connections, default %d.\n"
            "\n"
            "     -i      Starting interval, default %d.\n"
            "\n"
            "     -w      Count workers for tcp and udp, default _SC_NPROCESSORS_ONLN.\n"
            "\n"
            "     -e      Max peers per announce response, default %d.\n"
            "\n"
            "     -t      Socket timeout, default %d.\n"
            "\n"
            "     -k      Enable http's keep alive, default disable.\n"
            "\n"
            "     --charset\n"
            "             Charset for stats page only, default none.\n"
            "\n"
            "     --min-interval\n"
            "             Minimum interval to be reached based on load_avg, default %d.\n"
            "\n"
            "     --max-interval\n"
            "             Maximum interval to be reached based on load_avg, default %d.\n"
            "\n"
            "EXAMPLES\n"
            "\n"
            "     etracker\n"
            "     etracker -p 80\n"
            "     etracker --port 80\n"
            "     etracker -p 80 -i 600 -w 1 -e 400 -t 5 -k --charset utf-8 --min-interval 299 --max-interval 1799\n"
            "     etracker --help\n",
            DEFAULT_PORT,
            DEFAULT_INTERVAL,
            DEFAULT_MAX_PEER_PER_RESULT,
            DEFAULT_SOCKET_TIMEOUT,
            DEFAULT_MIN_INTERVAL,
            DEFAULT_MAX_INTERVAL
    );

    exit(223);
}
