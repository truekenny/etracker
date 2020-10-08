#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <unistd.h>
#include "argument.h"
#include "alloc.h"
#include "math.h"
#include "string.h"

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
#define NO_TCP_NAME                11
#define NO_UDP_NAME                12
#define LOCALE_NAME                13
#define NOFILE_NAME                14
#define CORE_NAME                  15
#define FAILED_NAME                16

#define DEFAULT_PORT                3000
#define DEFAULT_INTERVAL            1799
#define DEFAULT_MAX_PEER_PER_RESULT 60
#define DEFAULT_SOCKET_TIMEOUT      3
#define DEFAULT_KEEP_ALIVE          0
#define DEFAULT_MIN_INTERVAL        239u
#define DEFAULT_MAX_INTERVAL        1799u

unsigned int getName(char *name);

void showHelp();

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
            case NO_TCP_NAME:
                arguments->noTcp = 1;
                break;
            case NO_UDP_NAME:
                arguments->noUdp = 1;
                break;
            case LOCALE_NAME:
                arguments->locale = (index == argc - 1) ? NULL : argv[index + 1];
                break;
            case NOFILE_NAME:
                arguments->nofile = argumentValue;
                break;
            case CORE_NAME:
                arguments->core = argumentValue;
                break;
            case FAILED_NAME:
                arguments->failed = argumentValue;
                break;
        }
    }

    arguments->minInterval = max(DEFAULT_MIN_INTERVAL, arguments->minInterval);
    arguments->minInterval = min(DEFAULT_MAX_INTERVAL, arguments->minInterval);
    arguments->maxInterval = max(arguments->minInterval, arguments->maxInterval);
    arguments->maxInterval = min(DEFAULT_MAX_INTERVAL, arguments->maxInterval);

    if (!arguments->port
        || !arguments->interval
        || !arguments->workers
        || !arguments->maxPeersPerResponse
        || !arguments->socketTimeout
        || !arguments->minInterval
        || !arguments->maxInterval
        || (arguments->noTcp && arguments->noUdp)
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
    } else if (!strcmp(name, "--no-tcp")) {
        return NO_TCP_NAME;
    } else if (!strcmp(name, "--no-udp")) {
        return NO_UDP_NAME;
    } else if (!strcmp(name, "--locale")) {
        return LOCALE_NAME;
    } else if (!strcmp(name, "--nofile")) {
        return NOFILE_NAME;
    } else if (!strcmp(name, "--core")) {
        return CORE_NAME;
    } else if (!strcmp(name, "-f")) {
        return FAILED_NAME;
    }

    return UNKNOWN_NAME;
}

void showHelp() {
    printf(
            BOLD "NAME\n" RESET
            "     " BOLD "etracker" RESET " -- Open-source BitTorrent tracker\n"
            "\n"
            BOLD "SYNOPSIS\n"
            "     etracker" RESET
            " [" BOLD "-p" RESET " " UNDERLINE "port" RESET "]"
            " [" BOLD "-i" RESET " " UNDERLINE "interval" RESET "]"
            " [" BOLD "-w" RESET " " UNDERLINE "workers" RESET "]"
            " [" BOLD "-e" RESET " " UNDERLINE "peers" RESET "]"
            " [" BOLD "-t" RESET " " UNDERLINE "timeout" RESET "]\n"
            "        "
            " [" BOLD "-k" RESET "]"
            " [" BOLD "--no-tcp" RESET "]"
            " [" BOLD "--no-udp" RESET "]"
            " [" BOLD "--charset" RESET " " UNDERLINE "charset" RESET "]"
            " [" BOLD "--locale" RESET " " UNDERLINE "locale" RESET "]\n"
            "        "
            " [" BOLD "--min-interval" RESET " " UNDERLINE "min-interval" RESET "]"
            " [" BOLD "--max-interval" RESET " " UNDERLINE "max-interval" RESET "]\n"
            "        "
            " [" BOLD "--core" RESET " " UNDERLINE "core" RESET "]"
            " [" BOLD "--nofile" RESET " " UNDERLINE "nofile" RESET "]"
            " [" BOLD "-h" RESET "]\n"
            "\n"
            BOLD "DESCRIPTION\n" RESET
            "     Run BitTorrent tracker with interfaces:\n"
            "     http://your_host:port/announce\n"
            "     udp://your_host:port/announce\n"
            "\n"
            "     The following options are available:\n"
            "\n"
            "     " BOLD "--port" RESET " " UNDERLINE "port" RESET "\n"
            "     " BOLD "-p" RESET " " UNDERLINE "port" RESET "\n"
            "             Port for http and udp connections, default %d.\n"
            "\n"
            "     " BOLD "--interval" RESET " " UNDERLINE "interval" RESET "\n"
            "     " BOLD "-i" RESET " " UNDERLINE "interval" RESET "\n"
            "             Starting interval, default %d.\n"
            "\n"
            "     " BOLD "--workers" RESET " " UNDERLINE "workers" RESET "\n"
            "     " BOLD "-w" RESET " " UNDERLINE "workers" RESET "\n"
            "             Count workers for tcp and udp, default _SC_NPROCESSORS_ONLN.\n"
            "\n"
            "     " BOLD "--peers" RESET " " UNDERLINE "peers" RESET "\n"
            "     " BOLD "-e" RESET " " UNDERLINE "peers" RESET "\n"
            "             Max peers per announce response, default %d.\n"
            "\n"
            "     " BOLD "--timeout" RESET " " UNDERLINE "timeout" RESET "\n"
            "     " BOLD "-t" RESET " " UNDERLINE "timeout" RESET "\n"
            "             Socket timeout, default %d.\n"
            "\n"
            "     " BOLD "--keep\n" RESET
            "     " BOLD "-k\n" RESET
            "             Enable http's keep alive, default disable.\n"
            "\n"
            "     " BOLD "--no-tcp\n" RESET
            "             Disable TCP, default TCP enable.\n"
            "\n"
            "     " BOLD "--no-udp\n" RESET
            "             Disable UDP, default UDP enable.\n"
            "\n"
            "     " BOLD "--charset" RESET " " UNDERLINE "charset" RESET "\n"
            "             Charset for stats page only, default none.\n"
            "             Only for stats page.\n"
            "\n"
            "     " BOLD "--locale" RESET " " UNDERLINE "locale" RESET "\n"
            "             Locale for stats page, default current terminal's locale.\n"
            "             If your stats page is a little broken, then I recommend\n"
            "             the `en_US.UTF-8` locale.\n"
            "\n"
            "     " BOLD "--min-interval" RESET " " UNDERLINE "min-interval" RESET "\n"
            "             Minimum interval to be reached based on load_avg, default %d.\n"
            "\n"
            "     " BOLD "--max-interval" RESET " " UNDERLINE "max-interval" RESET "\n"
            "             Maximum interval to be reached based on load_avg, default %d.\n"
            "\n"
            "     " BOLD "--core" RESET " " UNDERLINE "core" RESET "\n"
            "             Change soft rlimit core (`-1` mean change soft to hard).\n"
            "\n"
            "     " BOLD "--nofile" RESET " " UNDERLINE "nofile" RESET "\n"
            "             Change soft rlimit nofile.\n"
            "             May be useful if you choose the keep-alive option.\n"
            "             MacOS default 2500, maximum 10000.\n"
            "             Debian default 1000, recommended 64000.\n"
            "\n"
            "     " BOLD "--help\n"
            "     -h\n" RESET
            "             This help.\n"
            "\n"
            BOLD "EXAMPLES\n" RESET
            "     Run on default port:\n"
            "          etracker\n"
            "\n"
            "     Run on port 80:\n"
            "          etracker -p 80\n"
            "\n"
            "     Run with keep-alive, recommended expand nofile for more connection:\n"
            "          etracker -p 80 -k --nofile 10000\n"
            "\n"
            "     Run with special locale:\n"
            "          etracker -p 80 --locale en_US.UTF-8 --charset utf-8\n"
            "\n"
            "     Run with save CPU:\n"
            "          etracker --min-interval 1799\n"
            "\n"
            "     Another examples:\n"
            "          etracker --port 80 --no-tcp --max-interval 1799\n"
            "          etracker -p 80 -i 600 -w 1 -e 400 -t 5 -k\n"
            "          etracker --help\n"
            "\n"
            BOLD "AUTHOR\n" RESET
            "     Implemented by Truekenny.\n"
            "     Source code is available at " UNDERLINE "https://github.com/truekenny/etracker" RESET ".\n",
            DEFAULT_PORT,
            DEFAULT_INTERVAL,
            DEFAULT_MAX_PEER_PER_RESULT,
            DEFAULT_SOCKET_TIMEOUT,
            DEFAULT_MIN_INTERVAL,
            DEFAULT_MAX_INTERVAL
    );

    exit(223);
}
