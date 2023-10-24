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
    int received_orders = 0;
    while(received_orders < waiter->capacity){
        sem_wait(bar->sem_requested_orders);
        if(!bar->closed){
            pthread_mutex_lock(bar->requested_orders_mtx);
            for (size_t j = 0; j < bar->n_requested_orders; j++)
            {
                Order order = bar->requested_orders[j];
                if((order.id_order > 0) && (order.round == bar->round)){
                    order.id_waiter = waiter->waiter_id;
                    printf("\n[Waiter %d] Receiving order %d. (r%d)", waiter->waiter_id, order.id_order, order.round);
                    rotate_orders(waiter->capacity, waiter->orders);
                    waiter->orders[waiter->capacity-1] = order;
                    bar->requested_orders[j] = (Order){0};
                    received_orders++;
                    break;
                }
            }
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
        printf("\n[Waiter %d] Registering order %d. (r%d)", waiter->waiter_id, waiter->orders[i].id_order, waiter->orders[i].round);
        rotate_orders(bar->n_registered_orders, bar->registered_orders);
        bar->registered_orders[bar->n_registered_orders-1] = waiter->orders[i];
    }
    pthread_mutex_unlock(bar->registered_orders_mtx);
};

void deliver_orders(Waiter* waiter, Bar* bar){
    printf("\n[Waiter %d] Delivering...", waiter->waiter_id);
    for (size_t i = 0; i < waiter->capacity; i++)
    {
        pthread_mutex_lock(bar->delivered_orders_mtx);
        rotate_orders(bar->n_delivered_orders, bar->delivered_orders);
        Order order = waiter->orders[i];
        bar->delivered_orders[bar->n_delivered_orders-1] = order;
        printf("\n[Waiter %d] Delivering order %d to client %d. (r%d)", waiter->waiter_id, order.id_order, order.id_client, order.round);
        pthread_mutex_unlock(bar->delivered_orders_mtx);
        sem_post(bar->sem_delivered_orders[order.id_client-1]);
    }
};

void increment_round(Waiter* waiter, Bar* bar){
    printf("\n[Waiter %d] Taking round notes.", waiter->waiter_id);
    for (size_t i = 0; i < waiter->capacity; i++)
    {
        sem_post(bar->sem_rounds);
    }
}

void waiter_action(void* data){
    WaiterData* waiter_data = (WaiterData*) data;
    Waiter* waiter = waiter_data->waiter;
    Bar* bar = waiter_data->bar;
    while (!bar->closed)
    {
        receive_orders(waiter, bar);
        register_orders(waiter, bar);
        deliver_orders(waiter, bar);
        increment_round(waiter, bar);
    };
};
