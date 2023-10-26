#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include "types.h"

// External functions
void print_order(Order order);
void rotate_orders(int n_orders, Order *orders);


void receive_orders(Waiter* waiter, Bar* bar){
    // Here we deal with the cases when the number of effective clients doesn't match the waiter's capacity 
    int total_orders = waiter->capacity;
    int round_services = (waiter->clients / waiter->capacity);
    // If the number of clients doesn't fit the total capacity of the waiter, we flexibilize the capacity criteria, in order to fit the number of clients
    if (waiter->clients % waiter->capacity && !(waiter->service_control % (round_services+1)))
        total_orders = waiter->clients % waiter->capacity; // Once in a round, if the waiter have a remaining client to serve, we deal with it first

    int received_orders = 0;
    while(received_orders < total_orders){
        // Wait for each order to be requested by the clients
        sem_wait(bar->sem_requested_orders[waiter->waiter_id-1]);
        if(!bar->closed){
            pthread_mutex_lock(bar->requested_orders_mtx);
            for (size_t i = 0; i < bar->n_requested_orders; i++)
            {
                Order order = bar->requested_orders[waiter->waiter_id-1][i];
                if((order.id_waiter == waiter->waiter_id) && (order.round == bar->round)){
                    if(order.id_drink > 0)
                        printf("\n[Waiter %d] Receiving order %d. (round %d)", waiter->waiter_id, order.id_order, order.round);
                    rotate_orders(waiter->capacity, waiter->orders);
                    waiter->orders[waiter->capacity-1] = order;
                    bar->requested_orders[waiter->waiter_id-1][i] = (Order){0};
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
        Order order = waiter->orders[i];
        if (order.id_drink > 0){ // Here we only register the orders that contain a drink id
            printf("\n[Waiter %d] Registering order %d. (round %d)", waiter->waiter_id, order.id_order, order.round);
            // Rotate the requested orders queue, to keep the registering sequence history
            rotate_orders(bar->n_registered_orders, bar->registered_orders); 
            bar->registered_orders[bar->n_registered_orders-1] = order; // Inserting at the end, just to use the same rotating function
        }
    }
    pthread_mutex_unlock(bar->registered_orders_mtx);
};

void deliver_orders(Waiter* waiter, Bar* bar){
    for (size_t i = 0; i < waiter->capacity; i++)
    {
        Order order = waiter->orders[i];
        if (order.id_waiter == waiter->waiter_id) // Dealing with possible empty orders
        {
            pthread_mutex_lock(bar->delivered_orders_mtx);
            // Rotate the delivered orders queue, so that the oldest orders stay closer to the initial index
            rotate_orders(bar->n_delivered_orders, bar->delivered_orders);
            bar->delivered_orders[bar->n_delivered_orders - 1] = order; // Inserting at the end of the queue
            waiter->orders[i] = (Order){0}; // Here we replace that spot on the list with an empty order

            if (order.id_drink > 0) // Only prints if the client order a drink
                printf("\n[Waiter %d] Delivering order %d to client %d. (round %d)", waiter->waiter_id, order.id_order, order.id_client, order.round);

            sem_post(bar->sem_delivered_orders[order.id_client-1]); // Trigger client to take it's order
            pthread_mutex_unlock(bar->delivered_orders_mtx);
        }
    }
};

void increment_round(Waiter* waiter, Bar* bar){
    // Trigger bar to increment the total number of clients served this round
    printf("\n[Waiter %d] Taking notes at the bar.", waiter->waiter_id);
    waiter->service_control++; // Incrementing the total number of cycles completed
    for (size_t i = 0; i < waiter->capacity; i++)
        sem_post(bar->sem_rounds);
}


// Waiter routine
void waiter_action(void* data){
    // Unpacking the bar and client information
    WaiterData* waiter_data = (WaiterData*) data;
    Waiter* waiter = waiter_data->waiter;
    Bar* bar = waiter_data->bar;

    // Waiter functioning behaviour
    while (!bar->closed)
    {
        receive_orders(waiter, bar);
        register_orders(waiter, bar);
        deliver_orders(waiter, bar);
        increment_round(waiter, bar);
    };
};
