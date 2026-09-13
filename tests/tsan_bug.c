/* Bug class: data race. ThreadSanitizer (tsan) catches this. */
#include <pthread.h>

static long shared;

static void *worker(void *arg) {
    (void)arg;
    for (int i = 0; i < 100000; i++)
        shared++; /* unsynchronized read-modify-write from two threads */
    return NULL;
}

int main(void) {
    pthread_t t1, t2;
    pthread_create(&t1, NULL, worker, NULL);
    pthread_create(&t2, NULL, worker, NULL);
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    return shared == 200000 ? 0 : 1;
}
