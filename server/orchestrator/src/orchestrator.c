#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <stdio.h>              
#include <unistd.h>         
#include <sys/epoll.h>
#include <errno.h>
#include <signal.h>

#include "orchestrator/orchestrator.h"
#include "orchestrator/state/clients_table.h"
#include "orchestrator/net/conn.h"
#include "orchestrator/net/io.h"
#include "orchestrator/net/listen_socket.h"
#include "server-config.h"
#include "common/hashmap.h"
#include "common/signals.h"

static void shutdownServer(int listen_fd, int epoll_fd, struct HashTable *clients, FILE *log_file)
{
    // Close socket for every active connection 
    ht_close_all_sockets(clients);

    ht_destroy(clients);

    // Later I will have 2 more data structure that I will need to free  
    // TODO
    // ... 

    close(epoll_fd);
    close(listen_fd); 
    fclose(log_file);
}

int orchestrator_run(pid_t parent_pid)
{
    // Setup signal 
    if(signals_install(SIGUSR1) < 0) { return -1; }

    struct Orchestrator orch = {0};

    orch.log_file = fopen(ORCH_LOG_PATH, "a");
    if(!orch.log_file)
    {
        // Critical Error
        perror("[orchestrator] fopen"); 
        return -1; 
    }
    
    orch.parent_pid = parent_pid;
    orch.listen_fd = setupListenSocket();
    if(orch.listen_fd < 0) { return -1; }

    // Setup clients hashmap
    orch.clients = ht_create(sizeof(int), 1, sizeof(struct Client), 1, hash, HASH_TABLE_SIZE); 
    if(!orch.clients)
    {
        printf("[orchestrator] ht_create\n");
        close(orch.listen_fd);
        return -1;
    }

    // Here I will set up PORTS QUEUE and GAME QUEUE (LATER)
    // TODO 
    // ...

    // Setup EPOLL 
    struct epoll_event eventQueue[ORCH_MAX_EPOLL_EVENTS];
    orch.epoll_fd = epoll_create1(0);
    if(orch.epoll_fd < 0)
    {
        perror("[orchestrator] epoll_create1");
        close(orch.listen_fd);
        goto boot_fail;
    }

    // Register listening socket with EPOLL  
    struct epoll_event ev; 
    ev.events = EPOLLIN;
    ev.data.fd = orch.listen_fd;

    if(epoll_ctl(orch.epoll_fd, EPOLL_CTL_ADD, orch.listen_fd, &ev) < 0)
    {
        perror("[orchestrator] epoll_ctl");
        close(orch.epoll_fd);
        close(orch.listen_fd);
        goto boot_fail;
    }

    for(;;)
    {
        // Check if parent wants to terminate the process 
        if(signals_should_terminate())
        {
            // Gracefully terminate this process by invoking shutdownServer function   
            shutdownServer(orch.listen_fd, orch.epoll_fd, orch.clients, orch.log_file);
            break;
        }

        int events_ready = epoll_wait(orch.epoll_fd, eventQueue, ORCH_MAX_EPOLL_EVENTS, 2000);

        if(events_ready < 0)
        {
            if(errno == EINTR) continue; // not critical 
            perror("[orchestrator] epoll_wait");
            goto fail;
        }

        for(int i = 0; i < events_ready; i++)
        {
            struct epoll_event cur_event = eventQueue[i];

            // Check if it is listening file descriptor 
            if(cur_event.data.fd == orch.listen_fd)
            {
                int accepted = acceptConnections(orch.log_file, orch.listen_fd, orch.epoll_fd, orch.clients);

                if(accepted == -1)
                {
                    // Critical Server Error -> Do Graceful Shutdown 
                    printf("[orchestrator] acceptConnection failed.\n");
                    goto fail;
                }
                continue;
            }

            if(cur_event.events & (EPOLLERR | EPOLLHUP | EPOLLRDHUP))
            {
                // Peer disconnected 
                if(closeConnection(orch.log_file, orch.epoll_fd, cur_event.data.fd, orch.clients) < 0)
                {
                    // Critical Error happened shutdown server 
                    printf("[orchestrator] closeConnection failed.\n");
                    goto fail;
                }
                continue;
            }

            if(cur_event.events & EPOLLIN)
            {
                // Read data sent by the client 
                if(receiveData(orch.epoll_fd, cur_event.data.fd, orch.clients, orch.log_file) < 0)
                {
                    printf("[orchestrator] receiveData failed.\n");
                    goto fail;  
                }
            }

            if(cur_event.events & EPOLLOUT)
            {
                // Socket is writable 
                if(sendData(orch.log_file, orch.epoll_fd, cur_event.data.fd, orch.clients) < 0)
                {
                    printf("[orchestrator] sendData failed.\n");
                    goto fail;
                }
            }
        }
    }

    return 0;

fail:
    shutdownServer(orch.listen_fd, orch.epoll_fd, orch.clients, orch.log_file);
    return -1;

boot_fail:
    ht_destroy(orch.clients); // safe on NULL
    fclose(orch.log_file);
    return -1;
}