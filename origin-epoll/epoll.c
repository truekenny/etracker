#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netdb.h>
#include <assert.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <memory.h>

#ifdef __APPLE__
#define MSG_NOSIGNAL 0
#endif

#define MAX_EVENTS 10

int main(int argc, const char *argv[]) {

    // Macos automatically binds both ipv4 and 6 when you do this.
    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(9999);

    socklen_t addrlen = sizeof(addr);

    int listen_sock = socket(AF_INET, SOCK_STREAM, 0);
    assert(listen_sock != -1);

    int on = 1;
    setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
    if (bind(listen_sock, (struct sockaddr *) &addr, sizeof(addr)) == -1) {
        perror("bind");
        return 1;
    }
    assert(listen(listen_sock, 5) != -1);

    struct epoll_event ev, events[MAX_EVENTS];
    int conn_sock, nfds, epollfd;

    epollfd = epoll_create1(0);
    if (epollfd == -1) {
        perror("epoll_create1");
        exit(EXIT_FAILURE);
    }

    ev.events = EPOLLIN;
    ev.data.fd = listen_sock;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, listen_sock, &ev) == -1) {
        perror("epoll_ctl: listen_sock");
        exit(EXIT_FAILURE);
    }

    unsigned int recvCount = 0;

    for (;;) {
        nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1);
        if (nfds == -1) {
            perror("epoll_wait");
            exit(EXIT_FAILURE);
        }

        for (int n = 0; n < nfds; ++n) {
            if (events[n].data.fd == listen_sock) {
                printf("New con %d\n", events[n].data.fd);
                conn_sock = accept(listen_sock,
                                   (struct sockaddr *) &addr, &addrlen);
                if (conn_sock == -1) {
                    perror("accept");
                    exit(EXIT_FAILURE);
                }

                int flags = fcntl(conn_sock, F_GETFL, 0);
                if (flags < 0)
                    perror("Flags failed");
                fcntl(conn_sock, F_SETFL, flags | O_NONBLOCK);

                ev.events = EPOLLIN | EPOLLET;
                ev.data.fd = conn_sock;
                if (epoll_ctl(epollfd, EPOLL_CTL_ADD, conn_sock,
                              &ev) == -1) {
                    perror("epoll_ctl: conn_sock");
                    exit(EXIT_FAILURE);
                }
            } else {
                // do_use_fd(events[n].data.fd);
                printf("recv\n");
                recvCount++;

                char readBuffer[1024];
                memset(readBuffer, 0, sizeof(readBuffer));
                size_t readSize = recv(events[n].data.fd, readBuffer, sizeof(readBuffer), MSG_NOSIGNAL | MSG_PEEK);
                printf("recv PEEK %d %s", readSize, readBuffer);

                if(recvCount % 2 == 0) {
                    size_t readSize = recv(events[n].data.fd, readBuffer, sizeof(readBuffer), MSG_NOSIGNAL);
                    printf("recv LAST %d %s", readSize, readBuffer);
                }
            }
        }
    }

    return 0;
}