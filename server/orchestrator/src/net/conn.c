#include <stdio.h>          
#include <stdbool.h>        
#include <errno.h>          
#include <unistd.h>        
#include <sys/epoll.h>     
#include <sys/socket.h>     
#include <netinet/in.h>     
#include <arpa/inet.h>      
#include <stdint.h>         

#include "common/hashmap.h"
#include "common/util.h"
#include "server-config.h" 
#include "orchestrator/state/client.h"

int closeConnection(FILE *const log_file, int epoll_fd, int target_fd, struct HashTable *const active_clients)
{    
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

        log_error_fd(log_file, "Hash table removal failed or element not found (possible memory leak)", target_fd, 0);

        return 1; // Indicate potential memory leak 
    }

    close(target_fd);

    // Log Success 
    log_error_fd(log_file, "Connection closed by peer", target_fd, 0);

    // Flush log_file's buffer  
    fflush(log_file);

    return 0;
}

int acceptConnections(FILE *const log_file, int listen_fd, const int epoll_fd, struct HashTable *const active_clients)
{
    int accepted = 0;

    // Accept all pending connections 
    while(true)
    {
        // Initialize Client Structure
        struct Client new_client;
        new_client.buf_size = TCP_SEGMENT_SIZE;
        new_client.cur_size = 0;
        new_client.game_q_size = TCP_SEGMENT_SIZE;
        new_client.game_q_cur_size = 0;
        new_client.is_received = false; 
        new_client.ACK_sent = false;
        new_client.game_info_sent = false;
        new_client.game_q_ready = false;

        struct sockaddr_in c_addr;
        socklen_t ca_len = sizeof(c_addr);
        int conn_fd = accept4(listen_fd, (struct sockaddr *)&c_addr, &ca_len, SOCK_NONBLOCK | SOCK_CLOEXEC);

        if(conn_fd < 0)
        {
            if(errno == EAGAIN || errno == EWOULDBLOCK)
                // no more pending connections in line 
                break; 

            log_error(log_file, "[accept_connections] accept error", errno);

            continue;
        }

        new_client.fd = conn_fd;
        new_client.addr = c_addr;

        // Add connection to the hash map 
        if(ht__insert_internal(active_clients, &conn_fd, &new_client) < 0)
        {
            // hash table failed adding a new key value pair 

            log_error(log_file, "[accept_connections] ht__insert_internal failed", 0);

            close(conn_fd);
            continue;
        }

        // Add connection fd to epoll 
        struct epoll_event ev = {.data.fd = conn_fd, .events = EPOLLIN | EPOLLRDHUP | EPOLLHUP | EPOLLERR}; 
        if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, conn_fd, &ev) == -1)
        {
            int saved_errno = errno; 

            if(ht__remove_internal(active_clients, &conn_fd) != 1)
            {
                close(conn_fd);
                log_error(log_file, "[accept_connections] CRITICAL — unable to remove client from active_clients (state corruption possible)\n", saved_errno);
                accepted = -1;
                break;
            }
            
            close(conn_fd);

            log_error_fd(log_file, "[accept_connections] epoll_ctl error", conn_fd, saved_errno);

            continue;
        }

        // Log connected Client 
        char time[TIME_BUFFER_SIZE];
        getTime(time, TIME_BUFFER_SIZE);

        // Retrieve IP Address and Source Port 
        char ip_str[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &c_addr.sin_addr, ip_str, sizeof(ip_str));
        uint16_t c_port = ntohs(c_addr.sin_port);

        fprintf(log_file, "%s [accept_connections] CONNECT %s:%u fd=%d\n", time, ip_str, c_port, conn_fd);
        accepted++;
    }

    // Flush log_file's buffer 
    fflush(log_file);

    return accepted;
}