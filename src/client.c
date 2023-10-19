#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include "types.h"

void random_sleep(int max_time);
char print_drink(int client_id, int drink_id);
void print_order(Order order);
void rotate_orders(int n_orders, Order* orders);

void chat(Client* client, int max_chatting_time)
{
    printf("\n[Client %d] Chatting...", client->client_id);
    random_sleep(max_chatting_time);
    fflush(stdout);
};

void request_order(Client* client, Bar* bar){
    Order order = (Order){
        .round = client->round,
        .id_client = client->client_id,
        .id_drink = rand() % 6,
        .id_order = (rand() % 1000)+1
    };
    client->order = &order;

    pthread_mutex_lock(bar->requested_orders_mtx);
    printf("\n[Client %d] Making new order %d", client->client_id, order.id_order);
    rotate_orders(bar->n_requested_orders, bar->requested_orders);
    bar->requested_orders[bar->n_requested_orders-1] = order;
    pthread_mutex_unlock(bar->requested_orders_mtx);
    sem_post(bar->sem_requested_orders);
    fflush(stdout);
};

void wait_order(Client* client, Bar* bar){
    sem_wait(bar->sem_delivered_orders[client->client_id-1]);
    if(!bar->closed){
        printf("\n[Client %d] Waiting for order %d to be prepared.", client->client_id, client->order->id_order);
        pthread_mutex_lock(bar->delivered_orders_mtx);
        for (size_t i = 0; i < bar->n_delivered_orders; i++)
        {
            Order order = bar->delivered_orders[i];
            if((order.id_client) == (client->client_id)){
                client->order = &order;
                bar->delivered_orders[i] = (Order){0};
                printf("\n[Client %d] Order %d received.", client->client_id, order.id_order);
                fflush(stdout);
                break;
            }
        }
        pthread_mutex_unlock(bar->delivered_orders_mtx);
    } else {
        pthread_exit(NULL);
    }
};

void drink(Client* client, int max_consuming_time)
{
    print_drink(client->client_id, client->order->id_drink);
    random_sleep(max_consuming_time);
    fflush(stdout);
};

void client_action(void* data)
{
    ClientData* client_data = (ClientData*) data;
    Client* client = client_data->client;
    Bar* bar = client_data->bar;
    while (!bar->closed)
    {
        chat(client, bar->max_chatting_time);
        request_order(client, bar);
        wait_order(client, bar);
        drink(client, bar->max_consuming_time);
        client->round++;
        printf("\n[Client %d] Going for round %d.", client->client_id, client->round);
    };
}