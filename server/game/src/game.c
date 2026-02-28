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
#include "net/io.h"

#define MAX_EPOLL_EVENTS 100 // will change that and move to a dedicated header file

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

    int epoll_fd = epoll_create1(0);
    if(epoll_fd == 0)
    {
        // Error 
        // Will handle later 
    }

    struct epoll_event eventsQueue[MAX_EPOLL_EVENTS];

    struct epoll_event ev; 
    ev.events = EPOLLIN;
    ev.data.fd = listen_fd;

    if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listen_fd, &ev) < 0)
    {
        // TODO 

    }

    for(;;)
    {
        int events_ready = epoll_wait(epoll_fd, eventsQueue, MAX_EPOLL_EVENTS, 2000);
        
        if(events_ready < 0)
        {
            // Error 
            // TODO 
        }

        for(int i = 0; i < events_ready; i++)
        {
            struct epoll_event cur_event = eventsQueue[i];

            udp_read(listen_fd);
            
        }

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