#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>

#define ACTION_DELAY_US 200000  /* microseconds delay after each action */

int numBatterInBowl = 0;
int numEggInBowl = 0;
bool readyToEat = false;
pthread_mutex_t lock;
pthread_cond_t needIngredients, readyToBake, startEating;

void addBatter() {
    numBatterInBowl += 1;
    printf("Added batter (batter=%d)\n", numBatterInBowl);
    usleep(ACTION_DELAY_US);
}
void addEgg() {
    numEggInBowl += 1;
    printf("Added egg (eggs=%d)\n", numEggInBowl);
    usleep(ACTION_DELAY_US);
}
void heatBowl() {
    printf("Heating bowl\n");
    /* heating sets cake ready */
    readyToEat = true;
    usleep(ACTION_DELAY_US);
}
void eatCake() {
    printf("Eating cake\n");
    usleep(ACTION_DELAY_US);
}

/* batter adder: adds 1 batter when bowl needs ingredients */
void* batterAdder(void* arg) {
    pthread_mutex_lock(&lock);
    while (1) {
        while (numBatterInBowl >= 1 || readyToEat) {
            pthread_cond_wait(&needIngredients, &lock);
        }
        addBatter();
        /* notify heater that ingredients changed */
        pthread_cond_signal(&readyToBake);
    }
    /* unreachable */
    pthread_mutex_unlock(&lock);
    return NULL;
}

/* egg breaker: each thread adds one egg when bowl needs ingredients */
void* eggBreaker(void* arg) {
    pthread_mutex_lock(&lock);
    while (1) {
        while (numEggInBowl >= 2 || readyToEat) {
            pthread_cond_wait(&needIngredients, &lock);
        }
        addEgg();
        pthread_cond_signal(&readyToBake);
    }
    /* unreachable */
    pthread_mutex_unlock(&lock);
    return NULL;
}

/* bowl heater: waits until bowl has 1 batter and 2 eggs, then heats */
void* bowlHeater(void* arg) {
    pthread_mutex_lock(&lock);
    while (1) {
        while (numBatterInBowl < 1 || numEggInBowl < 2 || readyToEat) {
            pthread_cond_wait(&readyToBake, &lock);
        }
        heatBowl();
        /* wake eater */
        pthread_cond_signal(&startEating);
    }
    /* unreachable */
    pthread_mutex_unlock(&lock);
    return NULL;
}

/* cake eater: waits until cake ready, eats it, resets bowl and notifies adders */
void* cakeEater(void* arg) {
    pthread_mutex_lock(&lock);
    while (1) {
        while (!readyToEat) {
            pthread_cond_wait(&startEating, &lock);
        }
        eatCake();
        /* reset bowl for next cake */
        readyToEat = false;
        numBatterInBowl = 0;
        numEggInBowl = 0;
        /* let adders proceed */
        pthread_cond_broadcast(&needIngredients);
    }
    /* unreachable */
    pthread_mutex_unlock(&lock);
    return NULL;
}

int main(int argc, char *argv[]) {
    pthread_t batter, egg1, egg2, heater, eater;

    pthread_mutex_init(&lock, NULL);
    pthread_cond_init(&needIngredients, NULL);
    pthread_cond_init(&readyToBake, NULL);
    pthread_cond_init(&startEating, NULL);

    pthread_create(&batter, NULL, batterAdder, NULL);
    pthread_create(&egg1, NULL, eggBreaker, NULL);
    pthread_create(&egg2, NULL, eggBreaker, NULL);
    pthread_create(&heater, NULL, bowlHeater, NULL);
    pthread_create(&eater, NULL, cakeEater, NULL);

    /* run forever (threads are infinite loops) */
    while (1) sleep(1);

    /* cleanup (unreachable) */
    pthread_join(batter, NULL);
    pthread_join(egg1, NULL);
    pthread_join(egg2, NULL);
    pthread_join(heater, NULL);
    pthread_join(eater, NULL);
    pthread_mutex_destroy(&lock);
    pthread_cond_destroy(&needIngredients);
    pthread_cond_destroy(&readyToBake);
    pthread_cond_destroy(&startEating);
    return 0;
}