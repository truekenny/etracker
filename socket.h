#ifndef SC6_SOCKET_H
#define SC6_SOCKET_H

#include <arpa/inet.h>
#include "stats.h"

#ifdef __APPLE__
#define MSG_NOSIGNAL 0
#endif

#if !defined(htonll)
#if __BIG_ENDIAN__
#define htonll(x)   (x)
#define ntohll(x)   (x)
#else
#define htonll(x)   ((((uint64_t)htonl(x&0xFFFFFFFF)) << 32) + htonl(x >> 32))
#define ntohll(x)   ((((uint64_t)ntohl(x&0xFFFFFFFF)) << 32) + ntohl(x >> 32))
#endif
#endif

#define SOCKET_VERSION_IPV4_BIT  0b1u
#define SOCKET_VERSION_IPV6_BIT 0b10u

struct render {
    struct block *block;
    int code;
    char *message;
    size_t size;
    int canKeepAlive;
    unsigned short socketTimeout;
    struct stats *stats;
    char *charset;
    char *contentType;
    char *websocketKey;
};

unsigned char getIpVersion(struct in6_addr *in6Addr);

void setTimeout(int socket);

void renderHttpMessage(struct render *render);

ssize_t send_(int socket, void *message, size_t size, struct stats *stats, _Bool sentOnce);

#endif //SC6_SOCKET_H
