#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include "types.h"

void print_order(Order order);
void rotate_orders(int n_orders, Order *orders);

void receive_orders(Waiter* waiter, Bar* bar){
    for (size_t i = 0; i < waiter->capacity; i++)
    {
        sem_wait(bar->sem_requested_orders);
        if(!bar->closed){
            pthread_mutex_lock(bar->requested_orders_mtx);
            int index = bar->requested_orders_start;
            Order order = bar->requested_orders[index];
            order.id_waiter = waiter->waiter_id;
            printf("\n[Waiter %d] Receiving order %d.", waiter->waiter_id, order.id_order);

            rotate_orders(waiter->capacity, waiter->orders);
            waiter->orders[0] = order;
            bar->requested_orders_start--;
            pthread_mutex_unlock(bar->requested_orders_mtx);
        } else {
            pthread_exit(NULL);
        }
    }
};

void register_orders(Waiter* waiter, Bar* bar){
    pthread_mutex_lock(bar->registered_orders_mtx);
    for (size_t i = 0; i < waiter->capacity; i++)
    {
        printf("\n[Waiter %d] Registering order %d.", waiter->waiter_id, waiter->orders[i].id_order);
        bar->registered_orders[bar->registered_orders_index] = waiter->orders[i];
        bar->registered_orders_index++;
    }
    pthread_mutex_unlock(bar->registered_orders_mtx);
};

void deliver_orders(Waiter* waiter, Bar* bar){

    for (size_t i = waiter->capacity; i > 0; i--)
    {
        int index = i-1;
        pthread_mutex_lock(bar->delivered_orders_mtx);
        rotate_orders(bar->delivered_orders_max_size, bar->delivered_orders);
        bar->delivered_orders[0] = waiter->orders[index];
        bar->delivered_orders_start++;
        sem_post(bar->sem_delivered_orders);
        pthread_mutex_unlock(bar->delivered_orders_mtx);
        printf("\n[Waiter %d] Delivering order %d to client %d.", waiter->waiter_id, waiter->orders[index].id_order, waiter->orders[index].id_client);
    }
};


void waiter_action(void* data){
    WaiterData* waiter_data = (WaiterData*) data;
    Waiter* waiter = waiter_data->waiter;
    Bar* bar = waiter_data->bar;
    while (!bar->closed)
    {
        receive_orders(waiter, bar);
        register_orders(waiter, bar);
        deliver_orders(waiter, bar);
    };
};
