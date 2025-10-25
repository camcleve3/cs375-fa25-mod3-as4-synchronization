#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>

pthread_mutex_t lock;
pthread_cond_t reader_cv, writer_cv;
int reader_count = 0;
int writer_waiting = 0;
bool writer_active = false;
int shared_data = 0;

void* reader(void* arg) {
    int id = *(int*)arg;
    for (int i = 0; i < 5; ++i) {
        pthread_mutex_lock(&lock);
        while (writer_waiting > 0 || writer_active) {
            pthread_cond_wait(&reader_cv, &lock);
        }
        reader_count++;
        pthread_mutex_unlock(&lock);

        /* perform read */
        printf("Reader %d reads: %d\n", id, shared_data);
        usleep(100000);

        pthread_mutex_lock(&lock);
        reader_count--;
        if (reader_count == 0) {
            pthread_cond_signal(&writer_cv);
        }
        pthread_mutex_unlock(&lock);
        usleep(100000);
    }
    return NULL;
}

void* writer(void* arg) {
    int id = *(int*)arg;
    for (int i = 0; i < 3; ++i) {
        pthread_mutex_lock(&lock);
        writer_waiting++;
        while (reader_count > 0 || writer_active) {
            pthread_cond_wait(&writer_cv, &lock);
        }
        writer_waiting--;
        writer_active = true;
        pthread_mutex_unlock(&lock);

        /* perform write */
        shared_data += 1;
        printf("Writer %d writes: %d\n", id, shared_data);
        usleep(200000);

        pthread_mutex_lock(&lock);
        writer_active = false;
        /* prefer writers: wake one writer first, otherwise wake all readers */
        if (writer_waiting > 0) {
            pthread_cond_signal(&writer_cv);
        } else {
            pthread_cond_broadcast(&reader_cv);
        }
        pthread_mutex_unlock(&lock);
        usleep(100000);
    }
    return NULL;
}

int main(void) {
    const int NREADERS = 3;
    const int NWRITERS = 2;
    pthread_t rthreads[NREADERS], wthreads[NWRITERS];
    int rids[NREADERS], wids[NWRITERS];

    pthread_mutex_init(&lock, NULL);
    pthread_cond_init(&reader_cv, NULL);
    pthread_cond_init(&writer_cv, NULL);

    for (int i = 0; i < NREADERS; ++i) {
        rids[i] = i;
        pthread_create(&rthreads[i], NULL, reader, &rids[i]);
    }
    for (int i = 0; i < NWRITERS; ++i) {
        wids[i] = i;
        pthread_create(&wthreads[i], NULL, writer, &wids[i]);
    }

    for (int i = 0; i < NREADERS; ++i) pthread_join(rthreads[i], NULL);
    for (int i = 0; i < NWRITERS; ++i) pthread_join(wthreads[i], NULL);

    pthread_mutex_destroy(&lock);
    pthread_cond_destroy(&reader_cv);
    pthread_cond_destroy(&writer_cv);
    return 0;
}
