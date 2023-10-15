#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include "types.h"

void random_sleep(int max_time)
{
    // int random_time = (rand() % (max_time));
    sleep(max_time);
};

int get_threads_length(pthread_t *threads){
    int n_threads = (int) sizeof(threads) / (int) sizeof(pthread_t);
    return n_threads;
};

int get_orders_length(Order *orders)
{
    int n_orders = (int)sizeof(orders) / (int)sizeof(Order);
    return n_orders;
};

void rotate_orders(Order* orders){
    int n_orders = get_orders_length(orders);
    for (size_t i = n_orders; i > 0; i--)
    {
        orders[i] = orders[i-1];
    }
};

void *client_action(void *n);
void create_client_threads(pthread_t *threads, Client *clients, Bar *bar)
{
    int n_threads = get_threads_length(threads);
    printf("%d", n_threads);
    for (size_t i = 0; i < n_threads; i++)
    {
        ClientData client_data = {.client = clients[i], .bar = *bar};
        pthread_create(&threads[i], NULL, client_action, (void*) &client_data);
    }
};

void *waiter_action(void *n);
void create_waiter_threads(pthread_t *threads, Waiter *waiters, Bar *bar)
{
    int n_threads = get_threads_length(threads);
    printf("%d", n_threads);
    for (size_t i = 0; i < n_threads; i++)
    {
        WaiterData waiter_data = {.waiter = waiters[i], .bar = *bar};
        pthread_create(&threads[i], NULL, waiter_action, (void *) &waiter_data);
    }
};

void finish_client_threads(pthread_t* threads, Client* clients)
{
    int n_threads = get_threads_length(threads);
    for (size_t i = 0; i < n_threads; i++)
    {
        pthread_join(threads[i], NULL);
    }
};

void finish_waiter_threads(pthread_t *threads, Waiter *waiters)
{
    int n_threads = get_threads_length(threads);
    for (size_t i = 0; i < n_threads; i++)
    {
        pthread_join(threads[i], NULL);
        sem_destroy(&waiters[i].sem_waiter);
    }
};
