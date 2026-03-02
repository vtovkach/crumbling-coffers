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

extern _Atomic bool stop_net; // Parent signals when to stop netThread  

_Atomic bool net_dead = false;

void *netThread(void *arg)
{   
    struct NetThreadArgs args = *(struct NetThreadArgs *)arg; 
    int listen_fd = -1; 
    int epoll_fd = -1;
    
    // Detach from the parent 
    pthread_detach(pthread_self());
    
    listen_fd = make_udp_server_socket(args.udp_port);
    if(listen_fd == -1)
    {
        // Something is wrong
        printf("[net thread] make_udp_server_socket failed.\n");
        goto exit;
    }

    epoll_fd = epoll_create1(0);
    if(epoll_fd == 0)
    {
        // Epoll initialization failed  
        perror("[net thread] epoll_create");
        goto exit; 
    }

    struct epoll_event eventsQueue[GM_MAX_EPOLL_EVENTS];

    struct epoll_event ev; 
    ev.events = EPOLLIN;
    ev.data.fd = listen_fd;

    if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listen_fd, &ev) < 0)
    {
        // Failed registering listen_fd with epoll 
        perror("[net thread] epoll_ctl (listen_fd)");
        goto exit;
    }

    int tick = 0;
    for(;;)
    {
        if(stop_net)
        {
            printf("[net thread] parent requests termination.\n");
            goto exit;  
        }

        int events_ready = epoll_wait(epoll_fd, eventsQueue, GM_MAX_EPOLL_EVENTS, 1);
        if(events_ready < 0)
        {
            if(errno == EINTR) {continue; } // Safe to continue 

            perror("[net thread] epoll_wait");
            goto exit; 
        }

        // I have only one listening file descriptor 
        // No need for the loop 
        if(events_ready > 0)
        {
            struct epoll_event cur_event = eventsQueue[0];
            udp_read(cur_event.data.fd);
        }
        
        sleep(2);
        printf("[net thread] Tick: %d\n", tick++);
    }

exit: 
    if(epoll_fd != -1)  { close(epoll_fd);  }
    if(listen_fd != -1) { close(listen_fd); }
    free(arg);
    net_dead = true;
    return NULL;
}