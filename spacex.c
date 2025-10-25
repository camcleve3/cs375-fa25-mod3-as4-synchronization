#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cv = PTHREAD_COND_INITIALIZER;
int n = 3;

void* counter(void* arg) {
    pthread_mutex_lock(&lock);
    while (n > 0) {
        printf("%d\n", n);
        fflush(stdout);
        n--;
        pthread_cond_signal(&cv);     // wake announcer each decrement
        pthread_mutex_unlock(&lock);
        usleep(10000);                // small delay to allow scheduling
        pthread_mutex_lock(&lock);
    }
    // ensure announcer is woken if waiting for n == 0
    pthread_cond_signal(&cv);
    pthread_mutex_unlock(&lock);
    return NULL;
}

void* announcer(void* arg) {
    pthread_mutex_lock(&lock);
    while (n != 0) {
        pthread_cond_wait(&cv, &lock);
    }
    printf("FALCON HEAVY TOUCH DOWN!\n");
    pthread_mutex_unlock(&lock);
    return NULL;
}

int main(void) {
    pthread_t t1, t2;

    if (pthread_create(&t1, NULL, counter, NULL) != 0) {
        perror("pthread_create");
        return 1;
    }
    if (pthread_create(&t2, NULL, announcer, NULL) != 0) {
        perror("pthread_create");
        return 1;
    }

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    pthread_mutex_destroy(&lock);
    pthread_cond_destroy(&cv);
    return 0;
}