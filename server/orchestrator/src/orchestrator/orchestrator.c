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
#include "orchestrator/matchmaker/game_queue.h"
#include "orchestrator/matchmaker/port_manager.h"
#include "server-config.h"
#include "ds/hashmap.h"
#include "signals.h"
#include "log_system.h"

static uint64_t id_counter = 0;

uint16_t port_counter = 0;

static void shutdownServer(int listen_fd, 
                           int epoll_fd, 
                           struct HashTable *clients, 
                           FILE *log_file, 
                           struct GameQueue *gq,
                           struct PortManager *pm
                        )
{
    // Close socket for every active connection 
    ht_close_all_sockets(clients, epoll_fd);

    // dont close ht_clients before game queue
    freeGameQueue(gq);
    ht_destroy(clients);
    destroyPortManager(pm, log_file);

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

    uint32_t server_ip = get_client_ip(orch.listen_fd);

    // Setup clients hashmap
    orch.clients = ht_create(sizeof(int), 1, sizeof(struct Client), 1, hash, HASH_TABLE_SIZE); 
    if(!orch.clients)
    {
        printf("[orchestrator] ht_create failure\n");
        close(orch.listen_fd);
        return -1;
    }

    struct GameQueue *gq = createGameQueue();
    orch.gq = gq;
    if(!orch.gq)
    { 
        perror("[orchestrator] createGameQueue");
        close(orch.listen_fd);
        goto boot_fail;
    }

    // Setup PortManager 
    orch.pm = initPortManager(orch.log_file);
    if(!orch.pm)
    {
        printf("[orchestrator] initPortManager failure\n");
        close(orch.listen_fd);
        goto boot_fail; 
    }

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
            shutdownServer(orch.listen_fd, orch.epoll_fd, orch.clients, orch.log_file, orch.gq, orch.pm);
            break;
        }

        // Later also check if there is available port 
        if(gq_ready(orch.gq, PLAYERS_PER_MATCH))
        {
            uint16_t av_port = getPort(orch.pm, orch.log_file);
            if(av_port == INVALID_PORT)
            {
                printf("[formSession] critical error.\n");
                goto fail; 
            }
            
            if(formSession(orch.log_file, orch.gq, orch.epoll_fd, av_port, server_ip) == -1)
            {
                // Critical Error happened 
                printf("[formSession] critical error.\n");
                goto fail; 
            }

            if(spawnGameProcess(orch.log_file, av_port) == -1)
            {
                // Critical Error 
                printf("[spawnGameProcess] critical error.\n");
                goto fail; 
            }
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
                int accepted = acceptConnections(orch.log_file, 
                                                 orch.listen_fd, 
                                                 orch.epoll_fd, 
                                                 id_counter++, 
                                                 orch.clients);
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
                if(closeConnection(orch.log_file, orch.epoll_fd, cur_event.data.fd, orch.clients, orch.gq) < 0)
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
                if(receiveData(orch.epoll_fd, cur_event.data.fd, orch.clients, orch.gq, orch.log_file) < 0)
                {
                    printf("[orchestrator] receiveData failed.\n");
                    goto fail;  
                }
            }

            if(cur_event.events & EPOLLOUT)
            {
                // Socket is writable 
                if(sendData(orch.log_file, orch.epoll_fd, cur_event.data.fd, orch.clients, orch.gq) < 0)
                {
                    printf("[orchestrator] sendData failed.\n");
                    goto fail;
                }
            }
        }
    }

    return 0;

fail:
    shutdownServer(orch.listen_fd, orch.epoll_fd, orch.clients, orch.log_file, orch.gq, orch.pm);
    return -1;

boot_fail:
    if(orch.pm) destroyPortManager(orch.pm, orch.log_file);
    ht_destroy(orch.clients); // safe on NULL
    fclose(orch.log_file);
    return -1;
}