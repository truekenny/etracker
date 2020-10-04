#include "thread.h"

void pthreadSetName(pthread_t id, char *name) {
#ifdef __APPLE__
    if (id == NULL) {
        // unused
    }
    pthread_setname_np(name);
#else
    pthread_setname_np(id, name);
#endif
}
