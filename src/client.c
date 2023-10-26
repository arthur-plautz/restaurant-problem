#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include "types.h"

// External functions
void random_sleep(int max_time);
char print_drink(int client_id, int drink_id);
void print_order(Order order);
void rotate_orders(int n_orders, Order* orders);


void chat(Client* client, int max_chatting_time)
{
    printf("\n[Client %d] Chatting...", client->client_id);
    random_sleep(max_chatting_time); // Chatting for a random time
    fflush(stdout);
};

void request_order(Client* client, Bar* bar){
    Order order = (Order){
        .round = client->round,
        .id_client = client->client_id,
        .id_waiter = client->waiter_id,
        .id_order = (rand() % 1000)+1 // Random order id varying 1-1000
    };

    // Client randomly deciding if he wants to order a drink
    if (!(rand() % 5))
        order.id_drink = -1;
    else
        order.id_drink = rand() % 6; // Choosing randomly among 6 possible drinks
    client->order = &order;

    pthread_mutex_lock(bar->requested_orders_mtx);
    if(order.id_drink > 0)
        printf("\n[Client %d] Making new order %d to waiter %d. (round %d)", client->client_id, order.id_order, client->waiter_id, client->round);
    else
        printf("\n[Client %d] Decided to pass this round...", client->client_id);
    fflush(stdout);

    // Rotate the requested orders queue, so that the oldest orders stay closer to the initial index
    rotate_orders(bar->n_requested_orders, bar->requested_orders[client->waiter_id-1]);
    bar->requested_orders[client->waiter_id-1][bar->n_requested_orders-1] = order; // Inserting at the end of the queue

    // If the client already make an order this round, he needs to wait till the next round
    if(client->round == bar->round){
        sem_post(bar->sem_requested_orders[client->waiter_id-1]); // Here we just trigger the waiter if the order is valid this round
    }
    pthread_mutex_unlock(bar->requested_orders_mtx);
};

void wait_order(Client* client, Bar* bar){
    // It waits until the waiter trigger the client semaphore, signaling that the order is ready
    sem_wait(bar->sem_delivered_orders[(client->client_id-1)]);
    if(!bar->closed){
        // If this round client decided to pass, will not be a drink id on the order
        if(client->order->id_drink > 0)
            printf("\n[Client %d] Waiting for order %d to be prepared.", client->client_id, client->order->id_order);
    } else {
        pthread_exit(NULL);
    }
};

void receive_order(Client *client, Bar *bar)
{
    pthread_mutex_lock(bar->delivered_orders_mtx);
    for (size_t i = 0; i < bar->n_delivered_orders; i++)
    {
        // We search for the order in the delivered orders queue based on the client id
        Order order = bar->delivered_orders[i];
        if (order.id_client == client->client_id)
        {
            client->order = &order;
            bar->delivered_orders[i] = (Order){0}; // Here we replace that spot on the list with an empty order

            if (client->order->id_drink > 0) // Only prints if the client order a drink
                printf("\n[Client %d] Order %d received.", client->client_id, order.id_order);
            break;
        }
    }
    pthread_mutex_unlock(bar->delivered_orders_mtx);
    client->round++; // Here we increment the number of rounds that the client received
}

void drink(Client* client, int max_consuming_time)
{
    if(client->order->id_drink > 0){ // Only prints if the client order a drink
        print_drink(client->client_id, client->order->id_drink);
        random_sleep(max_consuming_time); // Drinking for a random time
    }
};


// Client routine
void client_action(void* data)
{
    // Unpacking the bar and client information
    ClientData* client_data = (ClientData*) data;
    Client* client = client_data->client;
    Bar* bar = client_data->bar;

    // Client functioning behaviour
    while (!bar->closed)
    {
        chat(client, bar->max_chatting_time);
        request_order(client, bar);
        wait_order(client, bar);
        receive_order(client, bar);
        drink(client, bar->max_consuming_time);
    };
}