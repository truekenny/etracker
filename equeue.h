#ifndef SC6_EQUEUE_H
#define SC6_EQUEUE_H

#define EQUEUE_EVENTS_EACH_LOOP 32

#ifdef __APPLE__

#include <sys/event.h>

#else
#include <sys/epoll.h>
#endif

struct Eevent {
#ifdef __APPLE__
    struct kevent evList[EQUEUE_EVENTS_EACH_LOOP];
#else
    struct epoll_event evList[EQUEUE_EVENTS_EACH_LOOP];
#endif
};

int initEqueue();

void addClientEqueue(int equeue, int clientSocket);

void deleteClientEqueue(int equeue, int clientSocket);

int checkEqueue(int equeue, struct Eevent *eevent);

int getSocketEqueue(struct Eevent *eevent, int index);

int isEof(struct Eevent *eevent, int index);

int isRead(struct Eevent *eevent, int index);

#endif //SC6_EQUEUE_H
