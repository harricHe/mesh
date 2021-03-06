#include<stdio.h>
#include<stdlib.h>
#include<pthread.h>
#include<include/util.h>
#include<include/node.h>
#include<include/worker.h>

#define NUM_WORKERS 10

const char ROLE_DEFAULT = 'N';
const char ROLE_SOURCE = 'Q';
const char ROLE_GOAL = 'Z';

const char TYPE_CONTENT = 'C';
const char TYPE_OK = 'O';
const char TYPE_NEIGHBOUR = 'N';

int port = 3333;// the port this node runs on
int role;// the role of this node
struct router *my_router;// the routing table of this node
char package_id_blacklist[256];// package ids are hashed so we only blacklist 256 ids at the same time

pthread_mutex_t mutex_neighbours = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_neighbours = PTHREAD_COND_INITIALIZER;

pthread_mutex_t mutex_router = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_router = PTHREAD_COND_INITIALIZER;

pthread_mutex_t mutex_blacklist = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_blacklist = PTHREAD_COND_INITIALIZER;

int parse_config(int argc, char *argv[]) {
    int opt;

    // default value for role
    role = ROLE_DEFAULT;

    while ((opt = getopt (argc, argv, "-qzh")) != -1)
        switch (opt) {
            case 'q':
                role = ROLE_SOURCE;
                break;
            case 'z':
                role = ROLE_GOAL;
                break;
            case 1:
                port = atoi(argv[1]);
                break;
            case 'h':
            default:
                printf("Format: mesh [port] [-q (for source node)|-z (for goal node)|-h (for displaying help)]\n");
                return 0;
    }

    return 1;
}

int main(int argc, char *argv[]) {
    int thread_counter = 0;
    int sockfd;
    int newsockfd;
    pthread_t workers[NUM_WORKERS];
    my_router = malloc(sizeof(struct router));

    // parse config
    if (!parse_config(argc, argv)) {
        return -1;
    }

    // set up this node
    sockfd = create_node(port);

    if (sockfd == -1) {
        return -1;
    }
    dbg("Erstellt");

    // init list of all connected neighbours
    LIST_INIT(&neighbour_head);

    // init blacklist
    package_id_blacklist[0] = 1;// without this, packages with ID 0 are blacklisted from the beginning

    while (1) {
        newsockfd = wait_for_connection(sockfd);// wait for a new node to connect
        pthread_create(&workers[thread_counter], NULL, worker_init, (void *)newsockfd);// create a new thread for handling this connection
        thread_counter += 1;
        if (thread_counter >= NUM_WORKERS) {
            thread_counter = 0;
        }
    }

    // clean up mutexes and conditions
    pthread_mutex_destroy(&mutex_neighbours);
    pthread_cond_destroy(&cond_neighbours);
    pthread_mutex_destroy(&mutex_router);
    pthread_cond_destroy(&cond_router);
    pthread_mutex_destroy(&mutex_blacklist);
    pthread_cond_destroy(&cond_blacklist);

    return 0;
}