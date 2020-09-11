#include <stddef.h>
#include "equeue.h"

int initEqueue() {
    return kqueue();
}

void addClientEqueue(int equeue, int clientSocket) {
    struct kevent kEvent;

    EV_SET(&kEvent, clientSocket, EVFILT_READ, EV_ADD | EV_CLEAR, 0, 0, NULL);
    kevent(equeue, &kEvent, 1, NULL, 0, NULL);
}

int checkEqueue(int equeue, struct Eevent *eevent) {
    return kevent(equeue, NULL, 0, eevent->evList, EVENTS_EACH_LOOP, NULL);
}

int getSocketEqueue(struct Eevent *eevent, int index) {
    return (int) eevent->evList[index].ident;
}

int isEof(struct Eevent *eevent, int index) {
    return eevent->evList[index].flags & EV_EOF;
}

int isRead(struct Eevent *eevent, int index) {
    return eevent->evList[index].filter == EVFILT_READ;
}
