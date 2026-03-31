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
#include "ds/hashmap.h"                 
#include "util.h" 
#include "log_system.h"                   
#include "orchestrator/net/conn.h"          
#include "orchestrator/matchmaker/game_queue.h"  

ssize_t tcp_read(FILE *log_file, int fd, void *dest, size_t dest_size)
{
    if(!dest || dest_size == 0)
    {
        log_error(log_file, "[tcp_read] incorrect input", 0);
        return -1;
    }

    ssize_t n = recv(fd, dest, dest_size, 0);
    if(n > 0)
        return n; 

    if(n == 0)
    {
        // Client disconnected 
        return -2;
    }

    if(errno == EAGAIN || errno == EWOULDBLOCK) return 0;

    if(errno == EINTR) return 0;

    log_error(log_file, "[tcp_read] recv error", errno);
    return -1;
}

int tcp_send(FILE *log_file, int fd, void *buf, size_t buf_size)
{
    if(!buf || buf_size == 0)
    {
        log_error(log_file, "[tcp_send] incorrect input", 0);
        return -1;
    }

    ssize_t n = send(fd, buf, buf_size, 0);
    if(n >= 0)
        return n; 

    if(errno == EINTR) 
        return 0;

    if(errno == EAGAIN || errno == EWOULDBLOCK)
        return 0;

    log_error(log_file, "[tcp_send] send error", errno);
    return -1;
}