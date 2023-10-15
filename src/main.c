#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include "types.h"

void finish_client_threads(pthread_t *threads, Client *clients);
void finish_waiter_threads(pthread_t *threads, Waiter *waiters);

void create_waiters(Waiter* waiters, int n_waiters, int waiter_capacity)
{
    for (size_t i = 0; i < n_waiters; i++)
    {
        Waiter waiter = {
            .waiter_id = i,
            .capacity = waiter_capacity
        };
        sem_init(&waiter.sem_waiter, 0, 0);
        waiters[i] = waiter;
    };
};
void create_waiter_threads(pthread_t *threads, Waiter *waiters, Bar *bar);

void create_clients(Client* clients, int n_clients, int max_chatting_time, int max_consuming_time)
{
    for (size_t i = 0; i < n_clients; i++)
    {
        Client client = {
            .client_id = i,
            .max_chatting_time = max_chatting_time,
            .max_consuming_time = max_consuming_time
        };
        clients[i] = client;
    };
};
void create_client_threads(pthread_t *threads, Client *clients, Bar *bar);



int main(int argc, char *argv[])
{
    if (argc < 6) {
        printf("Use: %s <clients> <waiters> <clients/waiter> <rounds> <max.chat> <max.consume>", argv[0]);
        return 1;
    }

    int n_clients = atoi(argv[1]);
    int n_waiters = atoi(argv[2]);
    int waiter_capacity = atoi(argv[3]);
    int rounds = atoi(argv[4]);
    int max_chatting_time = atoi(argv[5]);
    int max_consuming_time = atoi(argv[6]);

    Client clients[n_clients];
    create_clients(clients, n_clients, max_chatting_time, max_consuming_time);
    Waiter waiters[n_waiters];
    create_waiters(waiters, n_waiters, waiter_capacity);

    Order requested_orders[n_clients];
    Order registered_orders[(n_clients * rounds)];
    Order delivered_orders[n_clients];
    Bar bar = {
        .closed = 0,
        .rounds = rounds,
        .requested_orders = requested_orders,
        .registered_orders = registered_orders,
        .delivered_orders = delivered_orders
    };

    sem_init(&bar.sem_requested_orders, 0, 0);
    sem_init(&bar.sem_delivered_orders, 0, 0);

    pthread_t client_threads[n_clients];
    create_client_threads(client_threads, clients, &bar);

    pthread_t waiter_threads[n_waiters];
    create_waiter_threads(waiter_threads, waiters, &bar);

    finish_client_threads(client_threads, clients);
    finish_waiter_threads(waiter_threads, waiters);

    pthread_mutex_destroy(&bar.requested_orders_mtx);
    pthread_mutex_destroy(&bar.registered_orders_mtx);
    pthread_mutex_destroy(&bar.delivered_orders_mtx);

    sem_destroy(&bar.sem_requested_orders);
    sem_destroy(&bar.sem_delivered_orders);

    return 0;
}
