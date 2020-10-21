#include "thread.h"

#define THREAD_MAX_LENGTH 15

void pthreadSetName(pthread_t id, char *name) {
    assert(strlen(name) <= THREAD_MAX_LENGTH);

#ifdef __APPLE__
    if (id == NULL) {
        // unused
    }
    pthread_setname_np(name);
#else
    pthread_setname_np(id, name);
#endif
}
