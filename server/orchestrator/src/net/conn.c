#include <stdio.h>          
#include <stdbool.h>        
#include <errno.h>          
#include <unistd.h>        
#include <sys/epoll.h>     
#include <sys/socket.h>     
#include <netinet/in.h>     
#include <arpa/inet.h>      
#include <stdint.h>         
#include <time.h>
#include <stdlib.h>

#include "ds/hashmap.h"
#include "util.h"
#include "log_system.h"
#include "server-config.h" 
#include "orchestrator/state/client.h"
#include "orchestrator/matchmaker/game_queue.h"
#include "random.h"

int closeConnection(FILE *const log_file, 
                    int epoll_fd, 
                    int target_fd, 
                    struct HashTable *const active_clients)
{    
    // Retrieve client to remove from game queue 
    struct Client *client = ht__get_internal(active_clients, &target_fd, sizeof(int));
    if(!client)
    {
        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, target_fd, NULL);
        ht__remove_internal(active_clients, &target_fd);
        close(target_fd);

        log_error_fd(log_file, "ht__get_internal did not find target \
                                (critical error)", target_fd, 0);

        return -1; // Indicate critical error 
    }

    if(epoll_ctl(epoll_fd, EPOLL_CTL_DEL, target_fd, NULL) == -1)
    {
        // epoll_ctl failed  
        ht__remove_internal(active_clients, &target_fd);
        close(target_fd);

        log_error_fd(log_file, "epoll_ctl DEL failed", target_fd, errno);

        return -1; // Indicate critical error  
    }
    
    if(ht__remove_internal(active_clients, &target_fd) <= 0)
    {
        // Hash Table error or element not found 

        close(target_fd);

        log_error_fd(log_file, "Hash table removal failed or element not found \
                                (possible memory leak)", target_fd, 0);

        return 1; // Indicate potential memory leak 
    }

    close(target_fd);

    // Log Success 
    log_error_fd(log_file, "Connection closed by peer", target_fd, 0);

    return 0;
}

int acceptConnections(FILE *const log_file,
                      int listen_fd,
                      int epoll_fd,
                      struct HashTable *const active_clients)
{
    int accepted = 0;

    // Accept all pending connections
    while (true)
    {
        struct Client new_client;
        struct sockaddr_in c_addr;
        socklen_t ca_len = sizeof(c_addr);

        int conn_fd = accept4(listen_fd, (struct sockaddr *)&c_addr,
                              &ca_len, SOCK_NONBLOCK | SOCK_CLOEXEC);

        if (conn_fd < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                break;

            log_error(log_file, "[accept_connections] accept error", errno);
            continue;
        }

        clock_gettime(CLOCK_MONOTONIC, &new_client.ts);
        secure_random_bytes(&new_client.client_id, PLAYER_ID_SIZE);

        new_client.fd = conn_fd;
        new_client.addr = c_addr;

        new_client.recv_buf = malloc(TCP_SEGMENT_SIZE);
        new_client.recv_len = 0;
        new_client.recv_capacity = TCP_SEGMENT_SIZE;

        new_client.send_buf = malloc(TCP_SEGMENT_SIZE);
        new_client.send_len = 0;
        new_client.send_capacity = TCP_SEGMENT_SIZE;

        if (ht__insert_internal(active_clients, &conn_fd, &new_client) < 0)
        {
            log_error(log_file,
                      "[accept_connections] ht insert failed", 0);
            close(conn_fd);
            continue;
        }

        struct epoll_event ev = {
            .data.fd = conn_fd,
            .events = EPOLLIN | EPOLLRDHUP | EPOLLHUP | EPOLLERR
        };

        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, conn_fd, &ev) == -1)
        {
            int saved_errno = errno;

            if (ht__remove_internal(active_clients, &conn_fd) != 1)
            {
                close(conn_fd);
                log_error(log_file,
                          "[accept_connections] CRITICAL remove failed",
                          saved_errno);
                accepted = -1;
                break;
            }

            close(conn_fd);

            log_error_fd(log_file,
                         "[accept_connections] epoll_ctl error",
                         conn_fd, saved_errno);
            continue;
        }

        char ip_str[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &c_addr.sin_addr, ip_str, sizeof(ip_str));

        char msg[128];
        snprintf(msg, sizeof(msg),
                 "[accept_connections] CONNECT %s:%u fd=%d",
                 ip_str, ntohs(c_addr.sin_port), conn_fd);

        log_message(log_file, msg);

        accepted++;
    }

    return accepted;
}