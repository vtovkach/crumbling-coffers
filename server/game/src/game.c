#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdatomic.h>
#include <unistd.h>
#include <errno.h>
#include <sys/epoll.h>
#include <sys/types.h>

#include "net/udp_socket.h"

#define PORT 10001

static _Atomic bool stop_net = false;
static _Atomic bool net_dead = false;

void *netThread(void *arg)
{   
    uint16_t udp_port = *(uint16_t*)arg;

    // Detach from the parent 
    pthread_detach(pthread_self());

    int listen_fd = make_udp_server_socket(udp_port);
    if(listen_fd == -1)
    {
        // Something wrong TODO 
        // 
        printf("[netThread] make_udp_server failed.\n");
        free(arg);
        return NULL;
    }

    


    free(arg);
    return NULL;
}

int runGame(uint16_t port)
{
    // Setup UDP Networking Thread 
    pthread_t net_thread; 

    uint16_t *port_arg = malloc(sizeof(*port_arg));     
    if(!port)
    {
        perror("[game] malloc");
        return -1;
    }

    if(pthread_create(&net_thread, NULL, netThread, port_arg) != 0)
    {
        perror("[game] pthread_create");
        return -1;
    }
    
    // Here I will have a main game loop    
    // TODO ...

    int tick = 0;

    for(;;)
    {
        if(net_dead)
            break; 

        printf("[game] Tick: %d\n", tick++);

        sleep(2);
    }

    return 0; 
}