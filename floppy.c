#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct account_t {
    pthread_mutex_t lock;
    int balance;
    long uuid;
} account_t;

typedef struct transfer_args {
    account_t *donor;
    account_t *recipient;
    int amount;
} transfer_args;

void transfer(account_t *donor, account_t *recipient, int amount) {
    account_t *first = donor;
    account_t *second = recipient;

    /* consistent ordering: by uuid, break ties by pointer */
    if (donor->uuid > recipient->uuid ||
        (donor->uuid == recipient->uuid && donor > recipient)) {
        first = recipient;
        second = donor;
    }

    pthread_mutex_lock(&first->lock);
    pthread_mutex_lock(&second->lock);

    if (donor->balance < amount) {
        printf("Insufficient funds: account %ld has %d, tried to send %d\n",
               donor->uuid, donor->balance, amount);
    } else {
        donor->balance -= amount;
        recipient->balance += amount;
        printf("Transferred %d from account %ld to %ld\n",
               amount, donor->uuid, recipient->uuid);
        printf("Balances: %ld=%d, %ld=%d\n",
               donor->uuid, donor->balance, recipient->uuid, recipient->balance);
    }

    pthread_mutex_unlock(&second->lock);
    pthread_mutex_unlock(&first->lock);
}

void* thread_transfer(void *arg) {
    transfer_args *a = (transfer_args*)arg;
    transfer(a->donor, a->recipient, a->amount);
    return NULL;
}

int main(void) {
    account_t acc1, acc2;
    pthread_t t1, t2;
    transfer_args a1, a2;

    acc1.balance = 1000;
    acc1.uuid = 1;
    pthread_mutex_init(&acc1.lock, NULL);

    acc2.balance = 500;
    acc2.uuid = 2;
    pthread_mutex_init(&acc2.lock, NULL);

    a1.donor = &acc1; a1.recipient = &acc2; a1.amount = 200;
    a2.donor = &acc2; a2.recipient = &acc1; a2.amount = 100;

    pthread_create(&t1, NULL, thread_transfer, &a1);
    pthread_create(&t2, NULL, thread_transfer, &a2);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    pthread_mutex_destroy(&acc1.lock);
    pthread_mutex_destroy(&acc2.lock);
    return 0;
}
