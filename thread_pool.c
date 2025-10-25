#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define NUM_THREADS 4

typedef struct {
    void (*task)(int);
    int arg;
} Task;

typedef struct {
    Task *queue;
    int head, tail, count, size;
    pthread_mutex_t lock;
    pthread_cond_t not_empty;
    pthread_cond_t not_full;
} ThreadSafeQueue;

void queue_init(ThreadSafeQueue *q, int size) {
    q->queue = malloc(sizeof(Task) * size);
    if (!q->queue) {
        perror("malloc");
        exit(1);
    }
    q->head = q->tail = q->count = 0;
    q->size = size;
    pthread_mutex_init(&q->lock, NULL);
    pthread_cond_init(&q->not_empty, NULL);
    pthread_cond_init(&q->not_full, NULL);
}

void queue_push(ThreadSafeQueue *q, Task task) {
    pthread_mutex_lock(&q->lock);
    while (q->count == q->size) {
        pthread_cond_wait(&q->not_full, &q->lock);
    }
    q->queue[q->tail] = task;
    q->tail = (q->tail + 1) % q->size;
    q->count++;
    pthread_cond_signal(&q->not_empty);
    pthread_mutex_unlock(&q->lock);
}

/* pop returns 1 on success, 0 if queue empty */
int queue_pop(ThreadSafeQueue *q, Task *task) {
    pthread_mutex_lock(&q->lock);
    if (q->count == 0) {
        pthread_mutex_unlock(&q->lock);
        return 0;
    }
    *task = q->queue[q->head];
    q->head = (q->head + 1) % q->size;
    q->count--;
    pthread_cond_signal(&q->not_full);
    pthread_mutex_unlock(&q->lock);
    return 1;
}

void* worker(void *arg) {
    ThreadSafeQueue *q = (ThreadSafeQueue*)arg;
    while (1) {
        Task t;
        if (queue_pop(q, &t)) {
            t.task(t.arg);
        } else {
            /* no task; back off briefly to avoid busy loop */
            sleep(1);
        }
    }
    return NULL;
}

void sample_task(int arg) {
    printf("Task executed with arg: %d\n", arg);
    fflush(stdout);
    usleep(100000); /* simulate work */
}

int main(void) {
    ThreadSafeQueue q;
    queue_init(&q, 10);

    pthread_t threads[NUM_THREADS];
    for (int i = 0; i < NUM_THREADS; ++i) {
        if (pthread_create(&threads[i], NULL, worker, &q) != 0) {
            perror("pthread_create");
            return 1;
        }
    }

    for (int i = 0; i < 10; ++i) {
        Task t = { .task = sample_task, .arg = i };
        queue_push(&q, t);
    }

    /* allow tasks to execute, then exit */
    sleep(5);

    /* not strictly necessary in this simple demo, but cleanup would go here */
    return 0;
}