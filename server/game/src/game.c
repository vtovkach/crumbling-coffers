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
#include "net/net_thread.h"
#include "server-config.h"
#include "game.h"

extern _Atomic bool net_dead; 
_Atomic bool stop_net = false;

int runGame(uint16_t port)
{
    // Setup UDP Networking Thread 
    pthread_t net_thread; 

    // Create args structure for the network thread 
    struct NetThreadArgs *net_thread_args = malloc(sizeof(*net_thread_args));
    if(!net_thread_args)
    {
        perror("[game process/runGame] malloc");
        return -1;
    }

    net_thread_args->udp_port = port;

    if(pthread_create(&net_thread, NULL, netThread, net_thread_args) != 0)
    {
        perror("[game] pthread_create");
        free(net_thread_args);
        return -1;
    }
    
    // Here I will have a main game loop    
    // TODO ...

    int tick = 0;
    for(;;)
    {
        if(net_dead)
        {
            free(net_thread_args);
            break;
        }
        
        sleep(1);

        if(tick++ == 2) break; 
    }

    stop_net = true; 
    // Wait for network thread 
    pthread_join(net_thread, NULL);

    return 0; 
}