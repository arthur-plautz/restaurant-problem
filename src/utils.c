#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include "types.h"


// General utilities
void print_drink(int client_id, int drink_id){
    char drinks[6][10] = {"🍹", "🍺", "🥃", "🍾", "🍸", "🍷"};
    char drink_names[6][10] = {"tiki", "beer", "whisky", "champagne", "martini", "wine"};
    printf("\n[Client %d] Drinking %s %s", client_id, drink_names[drink_id], drinks[drink_id]);
}

void random_sleep(int max_time)
{
    int random_time = (rand() % (max_time));
    sleep(random_time/1000); // This converts the parameter in miliseconds and perform the sleep
};



// Order utilities
void rotate_orders(int n_orders, Order* orders){
    //
    for (int i = 0; i < n_orders-1; i++)
    {
        orders[i] = orders[i+1];
    }
};

void print_order(Order order)
{
    printf("\n{order=%d, client=%d, drink=%d, waiter=%d, round=%d}", order.id_order, order.id_client, order.id_drink, order.id_waiter, order.round);
}


// Client thread functions
void *client_action(void *n);
ClientData* create_client_threads(int n_threads, pthread_t *threads, Client *clients, Bar *bar)
{
    ClientData* client_data = malloc(sizeof(ClientData)*n_threads);

    for (size_t i = 0; i < n_threads; i++)
    {
        client_data[i] = (ClientData) {.client = &clients[i], .bar = bar};
        pthread_create(&threads[i], NULL, &client_action, (void*) &client_data[i]);
    }
    return client_data;
};

void finish_client_threads(int n_threads, pthread_t *threads, Client *clients)
{
    for (size_t i = 0; i < n_threads; i++)
    {
        pthread_join(threads[i], NULL);
    }
};


// Waiter thread functions
void *waiter_action(void *n);
WaiterData* create_waiter_threads(int n_threads, pthread_t *threads, Waiter *waiters, Bar *bar)
{
    WaiterData* waiter_data = malloc(sizeof(WaiterData)*n_threads);

    for (size_t i = 0; i < n_threads; i++)
    {
        waiter_data[i] = (WaiterData) {.waiter = &waiters[i], .bar = bar};
        pthread_create(&threads[i], NULL, &waiter_action, (void*) &waiter_data[i]);
    }
    return waiter_data;
};

void finish_waiter_threads(int n_threads, pthread_t *threads, Waiter *waiters)
{
    for (size_t i = 0; i < n_threads; i++)
    {
        pthread_join(threads[i], NULL);
        free(waiters[i].orders);
    }
};


// Allocate memory and initialize semaphore list
void initialize_semaphores(int n_sem, sem_t **sem_list)
{
    for (size_t i = 0; i < n_sem; i++)
    {
        sem_t *sem = malloc(sizeof(sem_t));
        sem_list[i] = sem;
        sem_init(sem, 0, 0);
    }
}

// Free the allocated memory for semaphores and destroy them
void finalize_semaphores(int n_sem, sem_t **sem_list)
{
    for (size_t i = 0; i < n_sem; i++)
    {
        free(sem_list[i]);
        sem_destroy(sem_list[i]);
    }
}
