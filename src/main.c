#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include "types.h"

void print_order(Order order);
void initialize_semaphores(int n_sem, sem_t **sem_list);
void finalize_semaphores(int n_sem, sem_t **sem_list);
void finish_client_threads(int n_threads, pthread_t *threads, Client *clients);
void finish_waiter_threads(int n_threads, pthread_t *threads, Waiter *waiters);

void create_waiters(Waiter* waiters, int n_waiters, int waiter_capacity)
{
    for (size_t i = 0; i < n_waiters; i++)
    {
        Order* orders = malloc(sizeof(Order)*waiter_capacity);
        Waiter waiter = {
            .waiter_id = i+1,
            .capacity = waiter_capacity,
            .orders = orders
        };
        waiters[i] = waiter;
    };
};
WaiterData* create_waiter_threads(int n_threads, pthread_t *threads, Waiter *waiters, Bar *bar);

void create_clients(Client* clients, int n_clients)
{
    for (size_t i = 0; i < n_clients; i++)
    {
        Order order;
        Client client = {
            .round = 1,
            .client_id = i+1,
            .order = &order
        };
        clients[i] = client;
    };
};
ClientData* create_client_threads(int n_threads, pthread_t *threads, Client *clients, Bar *bar);

void assign_waiters_to_clients(int waiter_capacity, int n_waiters, Waiter* waiters, int n_clients, Client* clients){
    
    int assigned_waiters = 0;
    int assigned_clients = 0;
    int remaining_clients = n_clients % waiter_capacity;

    while(assigned_clients < (n_clients-remaining_clients)){
        int index = assigned_waiters % n_waiters;
        for (size_t i = 0; i < waiters[index].capacity; i++)
        {
            clients[assigned_clients].waiter_id = waiters[index].waiter_id;
            waiters[index].clients++;
            assigned_clients++;
        }
        assigned_waiters++;
    }

    if (remaining_clients)
    {
        for (size_t i = assigned_clients; i < n_clients; i++)
        {
            int waiter_index = assigned_waiters % n_waiters;
            clients[i].waiter_id = waiters[waiter_index].waiter_id;
            waiters[waiter_index].clients++;
            assigned_clients++;
        }
    }
}

void initialize_requested_orders(int n_waiters, int n_clients, Order** requested_orders){
    for (size_t i = 0; i < n_waiters; i++)
    {
        Order *waiter_orders = malloc(sizeof(Order) * n_clients);
        requested_orders[i] = waiter_orders;
    }
}

void finalize_requested_orders(int n_waiters, Order **requested_orders){
    for (size_t i = 0; i < n_waiters; i++)
        free(requested_orders[i]);
}

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

    Order *registered_orders = malloc(sizeof(Order) * n_clients * rounds);
    pthread_mutex_t registered_orders_mtx;
    pthread_mutex_init(&registered_orders_mtx, NULL);
    
    Order **requested_orders = malloc(sizeof(Order) * n_waiters * n_clients);
    initialize_requested_orders(n_waiters, n_clients, requested_orders);
    pthread_mutex_t requested_orders_mtx;
    pthread_mutex_init(&requested_orders_mtx, NULL);
    sem_t** sem_requested_orders = malloc(sizeof(sem_t)*n_waiters);
    initialize_semaphores(n_waiters, sem_requested_orders);

    Order *delivered_orders = malloc(sizeof(Order) * n_clients);
    pthread_mutex_t delivered_orders_mtx;
    pthread_mutex_init(&delivered_orders_mtx, NULL);
    sem_t** sem_delivered_orders = malloc(sizeof(sem_t)*n_clients);
    initialize_semaphores(n_clients, sem_delivered_orders);

    sem_t sem_rounds;
    sem_init(&sem_rounds, 0, 0);

    Bar bar = {
        .round = 1,
        .rounds = rounds,
        .sem_rounds = &sem_rounds,

        .max_chatting_time = max_chatting_time,
        .max_consuming_time = max_consuming_time,

        .n_requested_orders = n_clients,
        .requested_orders = requested_orders,
        .requested_orders_mtx = &requested_orders_mtx,
        .sem_requested_orders = sem_requested_orders,

        .registered_orders = registered_orders,
        .n_registered_orders = (n_clients * rounds),
        .registered_orders_mtx = &registered_orders_mtx,

        .n_delivered_orders = n_clients,
        .delivered_orders = delivered_orders,
        .delivered_orders_mtx = &delivered_orders_mtx,
        .sem_delivered_orders = sem_delivered_orders
    };

    printf("\n\n[bar open]\n");

    Client clients[n_clients];
    pthread_t client_threads[n_clients];
    create_clients(clients, n_clients);

    Waiter waiters[n_waiters];
    pthread_t waiter_threads[n_waiters];
    create_waiters(waiters, n_waiters, waiter_capacity);

    assign_waiters_to_clients(waiter_capacity, n_waiters, waiters, n_clients, clients);
    WaiterData* waiter_data = create_waiter_threads(n_waiters, waiter_threads, waiters, &bar);
    ClientData* client_data = create_client_threads(n_clients, client_threads, clients, &bar);

    for (size_t i = 0; i < rounds; i++)
    {
        printf("\n\n[starting round %d]\n", bar.round);
        if(i > 0){
            pthread_mutex_lock(bar.requested_orders_mtx);
            for (size_t i = 0; i < n_waiters; i++){
                for (size_t j = 0; j < n_clients; j++)
                {
                    Order order = bar.requested_orders[i][j];
                    if(order.id_order > 0)
                        sem_post(bar.sem_requested_orders[order.id_waiter-1]);
                }
            }
            pthread_mutex_unlock(bar.requested_orders_mtx);
        }
        int counter = 0;
        while (counter < n_clients){
            sem_wait(bar.sem_rounds);
            counter++;
        }
        printf("\n\n[finishing round %d]\n", bar.round);
        bar.round++;
    }
    bar.closed = 1;
    printf("\n\n[bar closing...]\n");

    for (size_t i = 0; i < (n_waiters); i++)
        sem_post(bar.sem_requested_orders[i]);
    for (size_t i = 0; i < (n_clients); i++)
        sem_post(bar.sem_delivered_orders[i]);

    finish_waiter_threads(n_waiters, waiter_threads, waiters);
    free(waiter_data);

    finish_client_threads(n_clients, client_threads, clients);
    free(client_data);

    finalize_requested_orders(n_waiters, requested_orders);
    finalize_semaphores(n_waiters, sem_requested_orders);
    finalize_semaphores(n_clients, sem_delivered_orders);

    pthread_mutex_destroy(bar.requested_orders_mtx);
    pthread_mutex_destroy(bar.registered_orders_mtx);

    sem_destroy(bar.sem_rounds);

    free(bar.sem_delivered_orders);
    free(bar.sem_requested_orders);

    printf("\n\nRegistered Orders:");
    for (size_t i = 0; i < bar.n_registered_orders; i++)
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
