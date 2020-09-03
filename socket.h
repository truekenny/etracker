#ifndef SC6_SOCKET_H
#define SC6_SOCKET_H

#ifdef __APPLE__
#define MSG_NOSIGNAL 0
#endif

void setTimeout(int socket);

void sendMessage(int socket, int code, char *message, size_t size, int canKeepAlive);

ssize_t send_(int socket, void *message, size_t size);

#endif //SC6_SOCKET_H
