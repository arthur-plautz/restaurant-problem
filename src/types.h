
typedef struct {
    int id_client;
    int id_waiter;
    int id_drink;
} Order;

typedef struct {
    int client_id;
    int max_consuming_time;
    int max_chatting_time;
    Order order;
} Client;

typedef struct {
    int waiter_id;
    int capacity;
    Order* orders;

    sem_t sem_waiter;
} Waiter;

typedef struct {
    int rounds;
    int closed;

    Order* requested_orders;
    int requested_orders_start;
    pthread_mutex_t requested_orders_mtx;
    sem_t sem_requested_orders;

    Order* registered_orders;
    int registered_orders_index;
    pthread_mutex_t registered_orders_mtx;

    Order* delivered_orders;
    int delivered_orders_start;
    pthread_mutex_t delivered_orders_mtx;
    sem_t sem_delivered_orders;
} Bar;

typedef struct {
    Waiter waiter;
    Bar bar;
} WaiterData;

typedef struct {
    Client client;
    Bar bar;
} ClientData;
