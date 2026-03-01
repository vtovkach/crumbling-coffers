#include <stdio.h>            
#include <stdint.h>            
#include <stddef.h>            
#include <string.h>            
#include <errno.h>             
#include <unistd.h>            
#include <sys/types.h>         
#include <sys/socket.h>        
#include <sys/epoll.h>     
    
#include "server-config.h"          
#include "orchestrator/state/client.h"      
#include "common/hashmap.h"                 
#include "common/util.h"                    
#include "orchestrator/net/conn.h"          
#include "orchestrator/queue/game_queue.h"  

int receiveData(int epoll_fd, int target_fd, HashTable *const clients, FILE *const log_file)
{
    uint8_t temp_buf[TCP_SEGMENT_SIZE];
    ssize_t bytes = recv(target_fd, temp_buf, sizeof(temp_buf), 0);

    if(bytes < 0)
    {
        if(errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)
        {
            // No error: nothing available to read 
            return 0;
        }

        if(closeConnection(log_file, epoll_fd, target_fd, clients) < 0) { return -1; }

        return 1;
    }

    if(bytes == 0)
    {        
        if(closeConnection(log_file, epoll_fd, target_fd, clients) < 0) { return -1; } 

        return 1;
    }

    struct Client *client = ht__get_internal(clients, &target_fd, sizeof(int));
    if(!client)
    {
        // ht__get_internal did not find the active client 
        // Close connection and stop tracking it with epoll 
        close(target_fd);

        if(epoll_ctl(epoll_fd, EPOLL_CTL_DEL, target_fd, NULL) < 0) { return -1; }

        return 1; 
    }

    if((size_t) bytes > (client->buf_size - client->cur_size))
    {
        // Drop message
        // Message exceeds protocol's size 

        // Reset buffer so clinet has another chance to sent a request to join game message  

        client->cur_size = 0;
        client->is_received = false;
        client->ACK_sent = false;
        client->game_info_sent = false;  

        return 2;
    }
    
    memcpy(client->buffer + client->cur_size, temp_buf, bytes);

    client->cur_size += bytes;

    if(client->cur_size >= TCP_SEGMENT_SIZE)
    {
        client->is_received = true;

        // Update epoll flags
        struct epoll_event ev = {.data.fd = target_fd, .events = EPOLLOUT | EPOLLHUP | EPOLLERR | EPOLLRDHUP};
        if(epoll_ctl(epoll_fd, EPOLL_CTL_MOD, target_fd, &ev) < 0) 
        {
            ht__remove_internal(clients, &target_fd);
            return -1;
        }

        // Add client to the game queue 
        addClientToQueue(client);

        // Prepare ACK message into client's buffer
        char ack_msg[TCP_SEGMENT_SIZE];
        memset(ack_msg, 0, TCP_SEGMENT_SIZE);
        memcpy(ack_msg, "ACK", 3);

        client->cur_size = 0;
        // Copy ACK message into the client's buffer 
        memcpy(client->buffer, (uint8_t *)ack_msg, TCP_SEGMENT_SIZE);
    }

    return 0;
}

int sendData(FILE *const log_file, int epoll_fd, int target_fd, HashTable *const clients)
{
    struct Client *client = ht__get_internal(clients, &target_fd, sizeof(int));
    if(!client)
    {
        // Error: client not found 
        closeConnection(log_file, epoll_fd, target_fd, clients);
        return 1; 
    }

    if(client->ACK_sent && client->game_q_ready)
    {
        // Send game info 
        ssize_t bytes = send(target_fd, client->game_queue_info + client->game_q_cur_size, client->game_q_size - client->game_q_cur_size, MSG_NOSIGNAL);
        if(bytes < 0)
        {
            // Handle error exit code 
            if(errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) { return 0; } // Not critical 

            log_error_fd(log_file, "send failed", target_fd, errno);
            closeConnection(log_file, epoll_fd, target_fd, clients);
            return 1;
        }

        client->game_q_cur_size += (size_t)bytes;

        if(client->game_q_cur_size == client->game_q_size)
        {
            // Close Connection: nothing else to do with the client 
            closeConnection(log_file, epoll_fd, target_fd, clients);
        }

        return 0;
    }
    
    if(!client->ACK_sent)
    {
        ssize_t bytes = send(target_fd, client->buffer + client->cur_size, client->buf_size - client->cur_size, MSG_NOSIGNAL);
        if(bytes < 0)
        {
            // Handle error exit code 
            if(errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) { return 0; } // Not critical 

            log_error_fd(log_file, "send failed", target_fd, errno);
            closeConnection(log_file, epoll_fd, target_fd, clients);
            return 1;
        }

        client->cur_size += (size_t)bytes;

        if(client->cur_size == client->buf_size)
        {
            client->ACK_sent = true; 

            if(!client->game_q_ready)
            {
                // Update epoll events: remove EPOLLOUT until game_q_ready is false 
                struct epoll_event ev = {.data.fd = target_fd, .events = EPOLLHUP | EPOLLERR | EPOLLRDHUP };
                if(epoll_ctl(epoll_fd, EPOLL_CTL_MOD, target_fd, &ev) < 0) { return -1; } // Critical Error 
            }
        }
    }
    
    return 0;
}