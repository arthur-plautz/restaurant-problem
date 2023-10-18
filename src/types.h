
typedef struct {
    int id_client;
    int id_waiter;
    int id_drink;
    int id_order;
} Order;

typedef struct {
    int client_id;
    int max_consuming_time;
    int max_chatting_time;
    Order* order;
} Client;

typedef struct {
    int waiter_id;
    int capacity;
    Order* orders;
} Waiter;

typedef struct {
    int rounds;
    int closed;

    Order* requested_orders;
    int requested_orders_start;
    int requested_orders_max_size;
    pthread_mutex_t* requested_orders_mtx;
    sem_t* sem_requested_orders;

    Order* registered_orders;
    int registered_orders_index;
    int registered_orders_max_size;
    pthread_mutex_t* registered_orders_mtx;

    Order* delivered_orders;
    int delivered_orders_start;
    int delivered_orders_max_size;
    pthread_mutex_t* delivered_orders_mtx;
    sem_t* sem_delivered_orders;
} Bar;

typedef struct {
    Waiter* waiter;
    Bar* bar;
} WaiterData;

typedef struct {
    Client* client;
    Bar* bar;
} ClientData;
