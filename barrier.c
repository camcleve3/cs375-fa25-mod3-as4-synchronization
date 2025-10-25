#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#define NUM_THREADS 4

pthread_mutex_t lock;
pthread_cond_t cv;
int count = 0;
int generation = 0;

void barrier(void) {
    pthread_mutex_lock(&lock);
    int gen = generation;

    count++;
    if (count == NUM_THREADS) {
        /* last thread to arrive: advance generation and wake everyone */
        count = 0;
        generation++;
        pthread_cond_broadcast(&cv);
        pthread_mutex_unlock(&lock);
        return;
    }

    /* wait until generation changes (handles spurious wakeups and reuse) */
    while (gen == generation) {
        pthread_cond_wait(&cv, &lock);
    }
    pthread_mutex_unlock(&lock);
}

void* worker(void* arg) {
    int id = *(int*)arg;
    printf("Thread %d: Before barrier\n", id);
    barrier();
    printf("Thread %d: After barrier\n", id);
    return NULL;
}

int main(void) {
    pthread_t threads[NUM_THREADS];
    int ids[NUM_THREADS];

    pthread_mutex_init(&lock, NULL);
    pthread_cond_init(&cv, NULL);

    for (int i = 0; i < NUM_THREADS; ++i) {
        ids[i] = i;
        if (pthread_create(&threads[i], NULL, worker, &ids[i]) != 0) {
            perror("pthread_create");
            return 1;
        }
    }

    for (int i = 0; i < NUM_THREADS; ++i) {
        pthread_join(threads[i], NULL);
    }

    pthread_mutex_destroy(&lock);
    pthread_cond_destroy(&cv);
    return 0;
}