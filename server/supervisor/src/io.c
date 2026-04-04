#include <stdio.h>
#include <errno.h>
#include <sys/socket.h>

#include "log_system.h"                   

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