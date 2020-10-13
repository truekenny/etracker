#include <stddef.h>
#include "equeue.h"

#include <sys/time.h>

// 0.05 секунды (здесь не может быть >= 1 секунда)
#define EQUEUE_WAIT_TIMEOUT_MS 50

int initEqueue() {
#ifdef __APPLE__
    return kqueue();
#else
    return epoll_create1(0);
#endif
}

/**
 * @param equeue
 * @param clientSocket
 * @warning Нельзя добавлять serverSocket
 */
void addClientEqueue(int equeue, int clientSocket) {
#ifdef __APPLE__
    struct kevent kEvent;
    EV_SET(&kEvent, clientSocket, EVFILT_READ, EV_ADD | EV_CLEAR, 0, 0, NULL);
    kevent(equeue, &kEvent, 1, NULL, 0, NULL);
#else
    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLET;
    ev.data.fd = clientSocket;
    epoll_ctl(equeue, EPOLL_CTL_ADD, clientSocket, &ev);
#endif
}

/**
 * @param equeue
 * @param clientSocket
 * @warning Нельзя добавлять serverSocket
 */
void deleteClientEqueue(int equeue, int clientSocket) {
#ifdef __APPLE__
    // Автоматическое удаление
    struct kevent kEvent;
    EV_SET(&kEvent, clientSocket, EVFILT_READ, EV_DELETE, 0, 0, NULL);
    kevent(equeue, &kEvent, 1, NULL, 0, NULL);
#else
    struct epoll_event ev;
    epoll_ctl(equeue, EPOLL_CTL_DEL, clientSocket, &ev);
#endif
}

int checkEqueue(int equeue, struct Eevent *eevent) {
#ifdef __APPLE__
    struct timespec timespec;
    timespec.tv_sec = 0;
    timespec.tv_nsec = EQUEUE_WAIT_TIMEOUT_MS * 1000000;

    return kevent(equeue, NULL, 0, eevent->evList, EQUEUE_EVENTS_EACH_LOOP, &timespec);
#else
    return epoll_wait(equeue, eevent->evList, EQUEUE_EVENTS_EACH_LOOP, EQUEUE_WAIT_TIMEOUT_MS);
#endif
}

int getSocketEqueue(struct Eevent *eevent, int index) {
#ifdef __APPLE__
    return (int) eevent->evList[index].ident;
#else
    return eevent->evList[index].data.fd;
#endif
}

int isEof(struct Eevent *eevent, int index) {
#ifdef __APPLE__
    return eevent->evList[index].flags & EV_EOF;
#else
    return 0;
#endif
}

int isRead(struct Eevent *eevent, int index) {
#ifdef __APPLE__
    return eevent->evList[index].filter == EVFILT_READ;
#else
    return 1;
#endif
}
