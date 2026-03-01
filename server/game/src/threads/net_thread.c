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
#include "server-config.h"
#include "threads/game.h"

extern _Atomic bool stop_net;

_Atomic bool net_dead = false;

void *netThread(void *arg)
{   
    struct NetThreadArgs args = *(struct NetThreadArgs *)arg; 
    
    // Detach from the parent 
    pthread_detach(pthread_self());

    int listen_fd = make_udp_server_socket(args.udp_port);
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

    struct epoll_event eventsQueue[ORCH_MAX_EPOLL_EVENTS];

    struct epoll_event ev; 
    ev.events = EPOLLIN;
    ev.data.fd = listen_fd;

    if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listen_fd, &ev) < 0)
    {
        // TODO 

    }

    int tick = 0;

    for(;;)
    {

        if(stop_net)
        {
            break; 
        }

        int events_ready = epoll_wait(epoll_fd, eventsQueue, ORCH_MAX_EPOLL_EVENTS, 2000);
        
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
        
        sleep(2);
        printf("[net thread] Tick: %d\n", tick++);
    }

    free(arg);
    return NULL;
}