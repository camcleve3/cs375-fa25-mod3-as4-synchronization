#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

typedef struct account_t {
    pthread_mutex_t lock;
    int balance;
    long uuid;
    int priority; /* simulated current priority for this account (for donation) */
} account_t;

typedef struct transfer_args {
    account_t *donor;
    account_t *recipient;
    int amount;
    int thread_priority;
} transfer_args;

/* transfer with consistent lock ordering and simulated priority donation */
void transfer(account_t *donor, account_t *recipient, int amount, int thread_priority) {
    account_t *first = donor;
    account_t *second = recipient;

    /* order locks consistently to avoid deadlock */
    if (donor->uuid > recipient->uuid ||
        (donor->uuid == recipient->uuid && donor > recipient)) {
        first = recipient;
        second = donor;
    }

    /* lock first account */
    pthread_mutex_lock(&first->lock);
    /* simulate donation to first if requestor has higher priority */
    int saved_first_pr = first->priority;
    if (first->priority < thread_priority) {
        printf("[donate] thread pr=%d -> acct %ld (old pr=%d)\n",
               thread_priority, first->uuid, first->priority);
        first->priority = thread_priority;
    }

    /* lock second account */
    pthread_mutex_lock(&second->lock);
    int saved_second_pr = second->priority;
    if (second->priority < thread_priority) {
        printf("[donate] thread pr=%d -> acct %ld (old pr=%d)\n",
               thread_priority, second->uuid, second->priority);
        second->priority = thread_priority;
    }

    /* perform transfer */
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

    /* restore simulated priorities */
    second->priority = saved_second_pr;
    first->priority = saved_first_pr;

    pthread_mutex_unlock(&second->lock);
    pthread_mutex_unlock(&first->lock);
}

void* transfer_thread(void *arg) {
    transfer_args *p = (transfer_args*)arg;
    transfer(p->donor, p->recipient, p->amount, p->thread_priority);
    return NULL;
}

int main(void) {
    account_t acc1, acc2;
    pthread_t t1, t2;
    transfer_args a1, a2;

    acc1.balance = 1000;
    acc1.uuid = 1;
    acc1.priority = 1; /* low */
    pthread_mutex_init(&acc1.lock, NULL);

    acc2.balance = 500;
    acc2.uuid = 2;
    acc2.priority = 0; /* lower */
    pthread_mutex_init(&acc2.lock, NULL);

    /* simulate: thread 1 has high priority (2) transferring from acc1->acc2,
       thread 2 has low priority (1) transferring acc2->acc1 */
    a1.donor = &acc1; a1.recipient = &acc2; a1.amount = 200; a1.thread_priority = 2;
    a2.donor = &acc2; a2.recipient = &acc1; a2.amount = 100; a2.thread_priority = 1;

    pthread_create(&t1, NULL, transfer_thread, &a1);
    pthread_create(&t2, NULL, transfer_thread, &a2);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    pthread_mutex_destroy(&acc1.lock);
    pthread_mutex_destroy(&acc2.lock);
    return 0;
}