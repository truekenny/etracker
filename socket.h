#ifndef SC6_SOCKET_H
#define SC6_SOCKET_H

void setTimeout(int socket);

void sendMessage(int socket, int code, char *message, size_t size, int canKeepAlive);

ssize_t send_(int socket, void *message, size_t size);

#endif //SC6_SOCKET_H
