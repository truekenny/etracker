#ifndef SC6_EQUEUE_H
#define SC6_EQUEUE_H

#define EVENTS_EACH_LOOP 32

#include <sys/event.h>

struct Eevent {
    struct kevent evList[EVENTS_EACH_LOOP];
};

int initEqueue();

void addClientEqueue(int equeue, int clientSocket);

int checkEqueue(int equeue, struct Eevent *eevent);

int getSocketEqueue(struct Eevent *eevent, int index);

int isEof(struct Eevent *eevent, int index);

int isRead(struct Eevent *eevent, int index);

#endif //SC6_EQUEUE_H
