
typedef struct {
    int id_client;
    int id_waiter;
    int id_drink;
    int id_order;
    int round;
} Order;

typedef struct {
    int client_id;
    int waiter_id;
    int round;
    Order* order;
} Client;

typedef struct {
    int waiter_id;
    int capacity;
    int clients;
    int service_control;
    Order* orders;
} Waiter;

typedef struct {
    int closed;

    int max_consuming_time;
    int max_chatting_time;

    int round;
    int rounds;
    sem_t* sem_rounds;

    Order** requested_orders;
    int n_requested_orders;
    pthread_mutex_t* requested_orders_mtx;
    sem_t** sem_requested_orders;

    Order* registered_orders;
    int n_registered_orders;
    pthread_mutex_t* registered_orders_mtx;

    Order* delivered_orders;
    int n_delivered_orders;
    pthread_mutex_t* delivered_orders_mtx;
    sem_t** sem_delivered_orders;
} Bar;

typedef struct {
    Waiter* waiter;
    Bar* bar;
} WaiterData;

typedef struct {
    Client* client;
    Bar* bar;
} ClientData;
