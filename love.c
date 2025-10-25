// ...existing code...
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

pthread_mutex_t lock;
pthread_cond_t cv;
int subaru = 0;

void* helper(void* arg) {
    pthread_mutex_lock(&lock);
    subaru = 1;
    pthread_cond_signal(&cv);
    pthread_mutex_unlock(&lock);
    return NULL;
}

int main(void) {
    pthread_t thread;

    if (pthread_mutex_init(&lock, NULL) != 0) {
        perror("pthread_mutex_init");
        return 1;
    }
    if (pthread_cond_init(&cv, NULL) != 0) {
        perror("pthread_cond_init");
        pthread_mutex_destroy(&lock);
        return 1;
    }

    if (pthread_create(&thread, NULL, helper, NULL) != 0) {
        perror("pthread_create");
        pthread_cond_destroy(&cv);
        pthread_mutex_destroy(&lock);
        return 1;
    }

    pthread_mutex_lock(&lock);
    while (subaru != 1) {
        pthread_cond_wait(&cv, &lock);
    }
    if (subaru == 1) {
        printf("I love Emilia!\n");
    } else {
        printf("I love Rem!\n");
    }
    pthread_mutex_unlock(&lock);

    pthread_join(thread, NULL);
    pthread_cond_destroy(&cv);
    pthread_mutex_destroy(&lock);
    return 0;
}
// ...existing code...