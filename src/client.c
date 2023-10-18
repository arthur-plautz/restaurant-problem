#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include "types.h"

void random_sleep(int max_time);
char print_drink(int drink_id);
void print_order(Order order);
void rotate_orders(int n_orders, Order* orders);

void chat(Client* client)
{
    printf("\n[Client %d] Chatting...", client->client_id);
    fflush(stdout);
    random_sleep(client->max_chatting_time);
};

void request_order(Client* client, Bar* bar){
    Order order = (Order){.id_client = client->client_id, .id_drink = rand() % 6, .id_order = rand() % 10000};
    client->order = &order;
    printf("\n[Client %d] Making new order %d", client->client_id, order.id_order);
    fflush(stdout);

    pthread_mutex_lock(bar->requested_orders_mtx);
    rotate_orders(bar->requested_orders_max_size, bar->requested_orders);
    bar->requested_orders[0] = order;
    bar->requested_orders_start++;
    sem_post(bar->sem_requested_orders);
    pthread_mutex_unlock(bar->requested_orders_mtx);
};

void wait_order(Client* client, Bar* bar){
    printf("\n[Client %d] Waiting for order %d to be prepared.", client->client_id, client->order->id_order);
    int client_order = 0;
    while (!client_order)
    {
        sem_wait(bar->sem_delivered_orders);
        if(!bar->closed){
            pthread_mutex_lock(bar->delivered_orders_mtx);
            int index = bar->delivered_orders_start;
            Order order = bar->delivered_orders[index];
            if(order.id_client == client->client_id){
                printf("\n[Client %d] Order %d received.", client->client_id, order.id_order);
                client->order = &order;
                rotate_orders(bar->delivered_orders_max_size, bar->delivered_orders);
                bar->delivered_orders_start--;
                client_order++;
            }
            pthread_mutex_unlock(bar->delivered_orders_mtx);
        } else {
            pthread_exit(NULL);
        }
    }
};

void drink(Client* client)
{
    printf("\n[Client %d] Drinking ", client->client_id);
    print_drink(client->order->id_drink);
    random_sleep(client->max_consuming_time);
};

void client_action(void* data)
{
    ClientData* client_data = (ClientData*) data;
    Client* client = client_data->client;
    Bar* bar = client_data->bar;
    while (!bar->closed)
    {
        chat(client);
        request_order(client, bar);
        wait_order(client, bar);
        drink(client);
    };
}