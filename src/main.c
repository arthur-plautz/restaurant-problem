#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include "types.h"

void print_order(Order order);
void finish_client_threads(int n_threads, pthread_t *threads, Client *clients);
void finish_waiter_threads(int n_threads, pthread_t *threads, Waiter *waiters);

void create_waiters(Waiter* waiters, int n_waiters, int waiter_capacity)
{
    for (size_t i = 0; i < n_waiters; i++)
    {
        Order* orders = malloc(sizeof(Order)*waiter_capacity*2);
        Waiter waiter = {
            .waiter_id = i * 3,
            .capacity = waiter_capacity,
            .orders = orders
        };
        waiters[i] = waiter;
    };
};
WaiterData* create_waiter_threads(int n_threads, pthread_t *threads, Waiter *waiters, Bar *bar);

void create_clients(Client* clients, int n_clients, int max_chatting_time, int max_consuming_time)
{
    for (size_t i = 0; i < n_clients; i++)
    {
        Order order;
        Client client = {
            .client_id = i * 2,
            .max_chatting_time = max_chatting_time,
            .max_consuming_time = max_consuming_time,
            .order = &order
        };
        clients[i] = client;
    };
};
ClientData* create_client_threads(int n_threads, pthread_t *threads, Client *clients, Bar *bar);



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

    Order* requested_orders = malloc(sizeof(Order) * n_clients * 2);           // Not sure why multiplying by 2 is necessary, but it only works this way
    Order *registered_orders = malloc(sizeof(Order) * n_clients * rounds * 2); // Not sure why multiplying by 2 is necessary, but it only works this way
    Order *delivered_orders = malloc(sizeof(Order) * n_clients * 2);           // Not sure why multiplying by 2 is necessary, but it only works this way
    pthread_mutex_t requested_orders_mtx;
    pthread_mutex_t registered_orders_mtx;
    pthread_mutex_t delivered_orders_mtx;
    sem_t sem_requested_orders;
    sem_t sem_delivered_orders;

    Bar bar = {
        .closed = 0,
        .rounds = 0,
        .requested_orders = requested_orders,
        .requested_orders_max_size = n_clients,
        .requested_orders_mtx = &requested_orders_mtx,
        .requested_orders_start = -1,
        .sem_requested_orders = &sem_requested_orders,
        .registered_orders = registered_orders,
        .registered_orders_max_size = (n_clients * rounds),
        .registered_orders_mtx = &registered_orders_mtx,
        .delivered_orders = delivered_orders,
        .delivered_orders_max_size = n_clients,
        .delivered_orders_mtx = &delivered_orders_mtx,
        .delivered_orders_start = -1,
        .sem_delivered_orders = &sem_delivered_orders,
    };

    pthread_mutex_init(bar.requested_orders_mtx, NULL);
    pthread_mutex_init(bar.registered_orders_mtx, NULL);
    pthread_mutex_init(bar.delivered_orders_mtx, NULL);

    sem_init(bar.sem_requested_orders, 0, 0);
    sem_init(bar.sem_delivered_orders, 0, 0);

    printf("\n\n[bar open]\n");

    Client clients[n_clients];
    pthread_t client_threads[n_clients];
    create_clients(clients, n_clients, max_chatting_time, max_consuming_time);
    
    Waiter waiters[n_waiters];
    pthread_t waiter_threads[n_waiters];
    create_waiters(waiters, n_waiters, waiter_capacity);

    WaiterData* waiter_data = create_waiter_threads(n_waiters, waiter_threads, waiters, &bar);
    ClientData* client_data = create_client_threads(n_clients, client_threads, clients, &bar);

    int counter = 0;
    while (counter < bar.registered_orders_max_size){
        sleep(1);
        counter++;
    }
    bar.closed = 1;
    printf("\n\n[bar closing...]\n");

    for (size_t i = 0; i < (n_waiters * waiter_capacity); i++)
        sem_post(bar.sem_requested_orders);
    for (size_t i = 0; i < n_clients; i++)
        sem_post(bar.sem_delivered_orders);

    finish_waiter_threads(n_waiters, waiter_threads, waiters);
    free(waiter_data);

    finish_client_threads(n_clients, client_threads, clients);
    free(client_data);

    pthread_mutex_destroy(bar.requested_orders_mtx);
    pthread_mutex_destroy(bar.registered_orders_mtx);
    pthread_mutex_destroy(bar.delivered_orders_mtx);

    sem_destroy(bar.sem_requested_orders);
    sem_destroy(bar.sem_delivered_orders);

    printf("\nRegistered Orders:");
    for (size_t i = 0; i < bar.registered_orders_max_size; i++)
    {
        if(bar.registered_orders[i].id_order > 0){
            print_order(bar.registered_orders[i]);
        }
    }

    free(requested_orders);
    free(registered_orders);
    free(delivered_orders);

    printf("\n\n[bar closed]\n");

    return 0;
}
