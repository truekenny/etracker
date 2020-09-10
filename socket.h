#ifndef SC6_SOCKET_H
#define SC6_SOCKET_H

#include "stats.h"
#include "request.h"

#ifdef __APPLE__
#define MSG_NOSIGNAL 0
#endif

void setTimeout(int socket);

void renderHttpMessage(struct block *block, int code, char *message, size_t size, int canKeepAlive, struct stats *stats);

ssize_t send_(int socket, void *message, size_t size, struct stats *stats);

#endif //SC6_SOCKET_H
