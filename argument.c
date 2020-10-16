#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <unistd.h>
#include "argument.h"
#include "alloc.h"
#include "math.h"
#include "string.h"
#include "exit_code.h"

#define ARGUMENT_UNKNOWN_INDEX                0
#define ARGUMENT_PORT_INDEX                   1
#define ARGUMENT_INTERVAL_INDEX               2
#define ARGUMENT_WORKERS_INDEX                3
#define ARGUMENT_MAX_PEERS_PER_RESPONSE_INDEX 4
#define ARGUMENT_SOCKET_TIMEOUT_INDEX         5
#define ARGUMENT_KEEP_ALIVE_INDEX             6
#define ARGUMENT_HELP_INDEX                   7
#define ARGUMENT_CHARSET_INDEX                8
#define ARGUMENT_MIN_INTERVAL_INDEX           9
#define ARGUMENT_MAX_INTERVAL_INDEX          10
#define ARGUMENT_NO_TCP_INDEX                11
#define ARGUMENT_NO_UDP_INDEX                12
#define ARGUMENT_LOCALE_INDEX                13
#define ARGUMENT_NOFILE_INDEX                14
#define ARGUMENT_CORE_INDEX                  15
#define ARGUMENT_FAILED_INDEX                16
#define ARGUMENT_NO_LOCATIONS                17

#define ARGUMENT_PORT_DEFAULT                3000
#define ARGUMENT_INTERVAL_DEFAULT            1799
#define ARGUMENT_MAX_PEER_PER_RESULT_DEFAULT 60
#define ARGUMENT_SOCKET_TIMEOUT_DEFAULT      3
#define ARGUMENT_KEEP_ALIVE_DEFAULT          0
#define ARGUMENT_MIN_INTERVAL_DEFAULT        239u
#define ARGUMENT_MAX_INTERVAL_DEFAULT        1799u

unsigned int getName(char *name);

void showHelp();

struct arguments *parseArguments(int argc, char *argv[]) {
    // Первый аргумент не может быть числом – скорей всего используется старый формат
    if (argc > 1 && atoi(argv[1]))
        showHelp();

    struct arguments *arguments = c_calloc(1, sizeof(struct arguments));

    arguments->port = ARGUMENT_PORT_DEFAULT;
    arguments->interval = ARGUMENT_INTERVAL_DEFAULT;
    arguments->workers = sysconf(_SC_NPROCESSORS_ONLN);
    arguments->maxPeersPerResponse = ARGUMENT_MAX_PEER_PER_RESULT_DEFAULT;
    arguments->socketTimeout = ARGUMENT_SOCKET_TIMEOUT_DEFAULT;
    arguments->keepAlive = ARGUMENT_KEEP_ALIVE_DEFAULT;
    arguments->minInterval = ARGUMENT_MIN_INTERVAL_DEFAULT;
    arguments->maxInterval = ARGUMENT_MAX_INTERVAL_DEFAULT;

    for (int index = 0; index < argc; index++) {
        char *argumentName = argv[index];
        int argumentValue = (index == argc - 1) ? 0 : atoi(argv[index + 1]);

        switch (getName(argumentName)) {
            case ARGUMENT_PORT_INDEX:
                arguments->port = argumentValue;
                break;
            case ARGUMENT_INTERVAL_INDEX:
                arguments->interval = argumentValue;
                break;
            case ARGUMENT_WORKERS_INDEX:
                arguments->workers = argumentValue;
                break;
            case ARGUMENT_MAX_PEERS_PER_RESPONSE_INDEX:
                arguments->maxPeersPerResponse = argumentValue;
                break;
            case ARGUMENT_SOCKET_TIMEOUT_INDEX:
                arguments->socketTimeout = argumentValue;
                break;
            case ARGUMENT_KEEP_ALIVE_INDEX:
                arguments->keepAlive = 1;
                break;
            case ARGUMENT_HELP_INDEX:
                showHelp();
                break;
            case ARGUMENT_CHARSET_INDEX:
                arguments->charset = (index == argc - 1) ? NULL : argv[index + 1];
                break;
            case ARGUMENT_MIN_INTERVAL_INDEX:
                arguments->minInterval = argumentValue;
                break;
            case ARGUMENT_MAX_INTERVAL_INDEX:
                arguments->maxInterval = argumentValue;
                break;
            case ARGUMENT_NO_TCP_INDEX:
                arguments->noTcp = 1;
                break;
            case ARGUMENT_NO_UDP_INDEX:
                arguments->noUdp = 1;
                break;
            case ARGUMENT_LOCALE_INDEX:
                arguments->locale = (index == argc - 1) ? NULL : argv[index + 1];
                break;
            case ARGUMENT_NOFILE_INDEX:
                arguments->nofile = argumentValue;
                break;
            case ARGUMENT_CORE_INDEX:
                arguments->core = argumentValue;
                break;
            case ARGUMENT_FAILED_INDEX:
                arguments->failed = argumentValue;
                break;
            case ARGUMENT_NO_LOCATIONS:
                arguments->noLocations = 1;
                break;
        }
    }

    arguments->minInterval = max(ARGUMENT_MIN_INTERVAL_DEFAULT, arguments->minInterval);
    arguments->minInterval = min(ARGUMENT_MAX_INTERVAL_DEFAULT, arguments->minInterval);
    arguments->maxInterval = max(arguments->minInterval, arguments->maxInterval);
    arguments->maxInterval = min(ARGUMENT_MAX_INTERVAL_DEFAULT, arguments->maxInterval);

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
        return ARGUMENT_PORT_INDEX;
    } else if (!strcmp(name, "-i") || !strcmp(name, "--interval")) {
        return ARGUMENT_INTERVAL_INDEX;
    } else if (!strcmp(name, "-w") || !strcmp(name, "--workers")) {
        return ARGUMENT_WORKERS_INDEX;
    } else if (!strcmp(name, "-e") || !strcmp(name, "--peers")) {
        return ARGUMENT_MAX_PEERS_PER_RESPONSE_INDEX;
    } else if (!strcmp(name, "-t") || !strcmp(name, "--timeout")) {
        return ARGUMENT_SOCKET_TIMEOUT_INDEX;
    } else if (!strcmp(name, "-k") || !strcmp(name, "--keep")) {
        return ARGUMENT_KEEP_ALIVE_INDEX;
    } else if (!strcmp(name, "-h") || !strcmp(name, "--help")) {
        return ARGUMENT_HELP_INDEX;
    } else if (!strcmp(name, "--charset")) {
        return ARGUMENT_CHARSET_INDEX;
    } else if (!strcmp(name, "--min-interval")) {
        return ARGUMENT_MIN_INTERVAL_INDEX;
    } else if (!strcmp(name, "--max-interval")) {
        return ARGUMENT_MAX_INTERVAL_INDEX;
    } else if (!strcmp(name, "--no-tcp")) {
        return ARGUMENT_NO_TCP_INDEX;
    } else if (!strcmp(name, "--no-udp")) {
        return ARGUMENT_NO_UDP_INDEX;
    } else if (!strcmp(name, "--locale")) {
        return ARGUMENT_LOCALE_INDEX;
    } else if (!strcmp(name, "--nofile")) {
        return ARGUMENT_NOFILE_INDEX;
    } else if (!strcmp(name, "--core")) {
        return ARGUMENT_CORE_INDEX;
    } else if (!strcmp(name, "-f")) {
        return ARGUMENT_FAILED_INDEX;
    } else if (!strcmp(name, "--no-locations")) {
        return ARGUMENT_NO_LOCATIONS;
    }

    return ARGUMENT_UNKNOWN_INDEX;
}

void showHelp() {
    printf(
            STRING_BOLD "NAME\n" STRING_RESET
            "     " STRING_BOLD "etracker" STRING_RESET " -- Open-source BitTorrent tracker\n"
            "\n"
            STRING_BOLD "SYNOPSIS\n"
            "     etracker" STRING_RESET
            " [" STRING_BOLD "-p" STRING_RESET " " STRING_UNDERLINE "port" STRING_RESET "]"
            " [" STRING_BOLD "-i" STRING_RESET " " STRING_UNDERLINE "interval" STRING_RESET "]"
            " [" STRING_BOLD "-w" STRING_RESET " " STRING_UNDERLINE "workers" STRING_RESET "]"
            " [" STRING_BOLD "-e" STRING_RESET " " STRING_UNDERLINE "peers" STRING_RESET "]"
            " [" STRING_BOLD "-t" STRING_RESET " " STRING_UNDERLINE "timeout" STRING_RESET "]\n"
            "        "
            " [" STRING_BOLD "-k" STRING_RESET "]"
            " [" STRING_BOLD "--no-tcp" STRING_RESET "]"
            " [" STRING_BOLD "--no-udp" STRING_RESET "]"
            " [" STRING_BOLD "--charset" STRING_RESET " " STRING_UNDERLINE "charset" STRING_RESET "]"
            " [" STRING_BOLD "--locale" STRING_RESET " " STRING_UNDERLINE "locale" STRING_RESET "]\n"
            "        "
            " [" STRING_BOLD "--min-interval" STRING_RESET " " STRING_UNDERLINE "min-interval" STRING_RESET "]"
            " [" STRING_BOLD "--max-interval" STRING_RESET " " STRING_UNDERLINE "max-interval" STRING_RESET "]\n"
            "        "
            " [" STRING_BOLD "--core" STRING_RESET " " STRING_UNDERLINE "core" STRING_RESET "]"
            " [" STRING_BOLD "--nofile" STRING_RESET " " STRING_UNDERLINE "nofile" STRING_RESET "]"
            " [" STRING_BOLD "--no-locations" STRING_RESET "]"

            " [" STRING_BOLD "-h" STRING_RESET "]\n"
            "\n"
            STRING_BOLD "DESCRIPTION\n" STRING_RESET
            "     Run BitTorrent tracker with interfaces:\n"
            "     http://your_host:port/announce\n"
            "     udp://your_host:port/announce\n"
            "\n"
            "     The following options are available:\n"
            "\n"
            "     " STRING_BOLD "--port" STRING_RESET " " STRING_UNDERLINE "port" STRING_RESET "\n"
            "     " STRING_BOLD "-p" STRING_RESET " " STRING_UNDERLINE "port" STRING_RESET "\n"
            "             Port for http and udp connections, default %d.\n"
            "\n"
            "     " STRING_BOLD "--interval" STRING_RESET " " STRING_UNDERLINE "interval" STRING_RESET "\n"
            "     " STRING_BOLD "-i" STRING_RESET " " STRING_UNDERLINE "interval" STRING_RESET "\n"
            "             Starting interval, default %d.\n"
            "\n"
            "     " STRING_BOLD "--workers" STRING_RESET " " STRING_UNDERLINE "workers" STRING_RESET "\n"
            "     " STRING_BOLD "-w" STRING_RESET " " STRING_UNDERLINE "workers" STRING_RESET "\n"
            "             Count workers for tcp and udp, default _SC_NPROCESSORS_ONLN.\n"
            "\n"
            "     " STRING_BOLD "--peers" STRING_RESET " " STRING_UNDERLINE "peers" STRING_RESET "\n"
            "     " STRING_BOLD "-e" STRING_RESET " " STRING_UNDERLINE "peers" STRING_RESET "\n"
            "             Max peers per announce response, default %d.\n"
            "\n"
            "     " STRING_BOLD "--timeout" STRING_RESET " " STRING_UNDERLINE "timeout" STRING_RESET "\n"
            "     " STRING_BOLD "-t" STRING_RESET " " STRING_UNDERLINE "timeout" STRING_RESET "\n"
            "             Socket timeout, default %d.\n"
            "\n"
            "     " STRING_BOLD "--keep\n" STRING_RESET
            "     " STRING_BOLD "-k\n" STRING_RESET
            "             Enable http's keep alive, default disable.\n"
            "\n"
            "     " STRING_BOLD "--no-tcp\n" STRING_RESET
            "             Disable TCP, default TCP enable.\n"
            "\n"
            "     " STRING_BOLD "--no-udp\n" STRING_RESET
            "             Disable UDP, default UDP enable.\n"
            "\n"
            "     " STRING_BOLD "--charset" STRING_RESET " " STRING_UNDERLINE "charset" STRING_RESET "\n"
            "             Charset for stats page only, default none.\n"
            "             Only for stats page.\n"
            "\n"
            "     " STRING_BOLD "--locale" STRING_RESET " " STRING_UNDERLINE "locale" STRING_RESET "\n"
            "             Locale for stats page, default current terminal's locale.\n"
            "             If your stats page is a little broken, then I recommend\n"
            "             the `en_US.UTF-8` locale.\n"
            "\n"
            "     " STRING_BOLD "--min-interval" STRING_RESET " " STRING_UNDERLINE "min-interval" STRING_RESET "\n"
            "             Minimum interval to be reached based on load_avg, default %d.\n"
            "\n"
            "     " STRING_BOLD "--max-interval" STRING_RESET " " STRING_UNDERLINE "max-interval" STRING_RESET "\n"
            "             Maximum interval to be reached based on load_avg, default %d.\n"
            "\n"
            "     " STRING_BOLD "--core" STRING_RESET " " STRING_UNDERLINE "core" STRING_RESET "\n"
            "             Change soft rlimit core (`-1` mean change soft to hard).\n"
            "\n"
            "     " STRING_BOLD "--nofile" STRING_RESET " " STRING_UNDERLINE "nofile" STRING_RESET "\n"
            "             Change soft rlimit nofile.\n"
            "             May be useful if you choose the keep-alive option.\n"
            "             MacOS default 2500, maximum 10000.\n"
            "             Debian default 1000, recommended 64000.\n"
            "\n"
            "     " STRING_BOLD "--no-locations" STRING_RESET "\n"
            "             Disable locations features, default enable.\n"
            "             Relaited url: " STRING_UNDERLINE "http://host:port/map.html" STRING_RESET "\n"
            "\n"
            "     " STRING_BOLD "--help\n"
            "     -h\n" STRING_RESET
            "             This help.\n"
            "\n"
            STRING_BOLD "EXAMPLES\n" STRING_RESET
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
            STRING_BOLD "EXIT STATUS\n" STRING_RESET
            "     %3d     chdir\n"
            "     %3d     getcwd\n"
            "     %3d     socket tcp create\n"
            "     %3d     reuseaddr\n"
            "     %3d     bind tcp\n"
            "     %3d     tcp client thread\n"
            "     %3d     socket tcp end\n"
            "     %3d     size connect request\n"
            "     %3d     size connect response\n"
            "     %3d     size announce request\n"
            "     %3d     size announce head response\n"
            "     %3d     size announce peer response\n"
            "     %3d     socket udp create\n"
            "     %3d     bind udp\n"
            "     %3d     udp client thread\n"
            "     %3d     socket udp end\n"
            "     %3d     unused list\n"
            "     %3d     list wrong limit\n"
            "     %3d     list wrong hash length\n"
            "     %3d     list not empty\n"
            "     %3d     semaphore init\n"
            "     %3d     semaphore post\n"
            "     %3d     semaphore destroy\n"
            "     %3d     geoip memory\n"
            "     %3d     geoip wrong data\n"
            "     %3d     geoip test failed\n"
            "\n"
            STRING_BOLD "AUTHOR\n" STRING_RESET
            "     Implemented by Truekenny.\n"
            "     Source code is available at " STRING_UNDERLINE "https://github.com/truekenny/etracker" STRING_RESET ".\n",
            ARGUMENT_PORT_DEFAULT,
            ARGUMENT_INTERVAL_DEFAULT,
            ARGUMENT_MAX_PEER_PER_RESULT_DEFAULT,
            ARGUMENT_SOCKET_TIMEOUT_DEFAULT,
            ARGUMENT_MIN_INTERVAL_DEFAULT,
            ARGUMENT_MAX_INTERVAL_DEFAULT,

            EXIT_CODE_CHDIR,
            EXIT_CODE_CWD,
            EXIT_CODE_SOCKET_TCP_CREATE,
            EXIT_CODE_REUSEADDR,
            EXIT_CODE_BIND_TCP,
            EXIT_CODE_TCP_CLIENT_THREAD,
            EXIT_CODE_SOCKET_TCP_END,
            EXIT_CODE_SIZE_CONNECT_REQUEST,
            EXIT_CODE_SIZE_CONNECT_RESPONSE,
            EXIT_CODE_SIZE_ANNOUNCE_REQUEST,
            EXIT_CODE_SIZE_ANNOUNCE_HEAD_RESPONSE,
            EXIT_CODE_SIZE_ANNOUNCE_PEER_RESPONSE,
            EXIT_CODE_SOCKET_UDP_CREATE,
            EXIT_CODE_BIND_UDP,
            EXIT_CODE_UDP_CLIENT_THREAD,
            EXIT_CODE_SOCKET_UDP_END,
            EXIT_CODE_UNUSED_LIST,
            EXIT_CODE_LIST_WRONG_LIMIT,
            EXIT_CODE_LIST_WRONG_HASH_LENGTH,
            EXIT_CODE_LIST_NOT_EMPTY,
            EXIT_CODE_SEMAPHORE_INIT,
            EXIT_CODE_SEMAPHORE_POST,
            EXIT_CODE_SEMAPHORE_DESTROY,
            EXIT_CODE_GEOIP_MEMORY,
            EXIT_CODE_GEOIP_WRONG_DATA,
            EXIT_CODE_GEOIP_TEST_FAILED
    );

    exit(EXIT_SUCCESS);
}
