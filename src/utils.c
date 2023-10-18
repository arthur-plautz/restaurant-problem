#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include "types.h"

void print_drink(int drink_id){
    char drinks[6][10] = {"🍹", "🍺", "🥃", "🍾", "🍸", "🍷"};
    char drink_names[6][10] = {"tiki", "beer", "whisky", "champagne", "martini", "wine"};
    printf("%s %s", drink_names[drink_id], drinks[drink_id]);
}

void random_sleep(int max_time)
{
    int random_time = (rand() % (max_time));
    sleep(random_time);
};

void print_order(Order order){
    printf("\n{order=%d, client=%d, drink=%d, waiter=%d}", order.id_order, order.id_client, order.id_drink, order.id_waiter);
}

void rotate_orders(int n_orders, Order* orders){
    if(n_orders > 1) {
        for (int i = n_orders; i > 0; i--)
        {
            orders[i] = orders[i-1];
        }
    }
};

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

void finish_client_threads(int n_threads, pthread_t* threads, Client* clients)
{
    for (size_t i = 0; i < n_threads; i++)
    {
        pthread_join(threads[i], NULL);
    }
};

void finish_waiter_threads(int n_threads, pthread_t *threads, Waiter *waiters)
{
    for (size_t i = 0; i < n_threads; i++)
    {
        pthread_join(threads[i], NULL);
        free(waiters[i].orders);
    }
};
