#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include "types.h"

int get_orders_length(Order* orders);
void rotate_orders(Order *orders);

void receive_orders(Waiter* waiter, Bar* bar){
    int received_orders = 0;
    while (received_orders != waiter->capacity)
    {
        sem_wait(&bar->sem_requested_orders);
        pthread_mutex_lock(&bar->requested_orders_mtx);
        int index = bar->requested_orders_start;
        Order order = bar->requested_orders[index]; 
        order.id_waiter = waiter->waiter_id;
        waiter->orders[-1] = order;
        rotate_orders(bar->requested_orders);
        pthread_mutex_unlock(&bar->requested_orders_mtx);
    }
};

void register_orders(Waiter* waiter, Bar* bar){
    pthread_mutex_lock(&bar->registered_orders_mtx);
    for (size_t i = 0; i < waiter->capacity; i++)
    {
        bar->registered_orders_index++;
        bar->registered_orders[bar->registered_orders_index] = waiter->orders[i];
    }
    pthread_mutex_unlock(&bar->registered_orders_mtx);
};

void deliver_orders(Waiter* waiter, Bar* bar){
    for (size_t i = 0; i < waiter->capacity; i++)
    {
        pthread_mutex_lock(&bar->delivered_orders_mtx);
        rotate_orders(bar->delivered_orders);
        bar->delivered_orders[0] = waiter->orders[i];
        sem_post(&bar->sem_delivered_orders);
        pthread_mutex_unlock(&bar->delivered_orders_mtx);
    }
};

void waiter_action(void* data){
    WaiterData *waiter_data = data;
    Waiter waiter = (Waiter) waiter_data->waiter;
    Bar bar = waiter_data->bar;

    while (!bar.closed)
    {
        receive_orders(&waiter, &bar);
        register_orders(&waiter, &bar);
        deliver_orders(&waiter, &bar);
    };
};
