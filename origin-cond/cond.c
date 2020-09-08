// C program to implement cond(), signal()
// and wait() functions 
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

// Declaration of thread condition variable 
pthread_cond_t cond1 = PTHREAD_COND_INITIALIZER;

// declaring mutex 
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

int done = 0;

// Thread function 
void *foo() {

    int threadNumber = done;

    // printf("done S = %d\n", done);

    // acquire a lock
    pthread_mutex_lock(&lock);

    // printf("done E = %d\n", done);

    int number = ++done;

    if (number <= 2) {

        // let's wait on conition variable cond1
        // printf("Waiting on condition variable cond1 S\n");
        // Освобождает lock, ожидает cond1

        printf("before wait %d\n", number);
        pthread_cond_wait(&cond1, &lock);
        printf("after wait %d\n", number);
        pthread_mutex_unlock(&lock);
        sleep(1);
        printf("after wait sleep %d\n", number);
        // printf("Waiting on condition variable cond1 E\n");
    } else {

        // Let's signal condition variable cond1
        // printf("Signaling condition variable cond1 S\n");
        // Освобождает cond1
        printf("before signal %d\n", number);
        // pthread_cond_signal(&cond1);
        pthread_cond_broadcast(&cond1);
        pthread_mutex_unlock(&lock);
        printf("after signal %d\n", number);
        // printf("Signaling condition variable cond1 E\n");
    }

    // release lock

    // printf("Returning thread = %d\n", threadNumber);

    return NULL;
}

// Driver code 
int main() {
    pthread_t tid1, tid1_1, tid2;

    // Create thread 1 
    pthread_create(&tid1, NULL, foo, NULL);
    pthread_create(&tid1, NULL, foo, NULL);

    // sleep for 1 sec so that thread 1 
    // would get a chance to run first
    usleep(10000);
    // printf("sleep\n");
    sleep(1);

    // Create thread 2 
    pthread_create(&tid2, NULL, foo, NULL);

    // wait for the completion of thread 2 
    pthread_join(tid2, NULL);
    pthread_join(tid1, NULL);
    //pthread_join(tid1_1, NULL);

    sleep(1);

    return 0;
}
