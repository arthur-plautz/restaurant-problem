#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include "types.h"

void random_sleep(int max_time);
int get_orders_length(Order* orders);
void rotate_orders(Order* orders);

void chat(Client* client)
{
    random_sleep(client->max_chatting_time);
};

void request_order(Client* client, Bar* bar){
    client->order = (Order) {.id_client = client->client_id};

    pthread_mutex_lock(&bar->requested_orders_mtx);
    bar->requested_orders[0] = client->order;
    pthread_mutex_unlock(&bar->requested_orders_mtx);

    sem_post(&bar->sem_requested_orders);
};

void wait_order(Client* client, Bar* bar){
    int client_order = 0;
    while (!client_order)
    {
        sem_wait(&bar->sem_delivered_orders);
        pthread_mutex_lock(&bar->delivered_orders_mtx);
        int index = bar->delivered_orders_start;
        Order order = bar->delivered_orders[index];
        if(order.id_client == client->client_id){
            client->order = order;
            rotate_orders(bar->delivered_orders);
            bar->delivered_orders_start++;
            client_order++;
        }
        pthread_mutex_unlock(&bar->delivered_orders_mtx);
    }
};

void drink(Client* client)
{
    random_sleep(client->max_consuming_time);
};

void client_action(void* data)
{
    ClientData* client_data = data;
    Client client = (Client) client_data->client;
    Bar bar = client_data->bar;

    while (!bar.closed)
    {
        chat(&client);
        request_order(&client, &bar);
        wait_order(&client, &bar);
        drink(&client);
    };
}