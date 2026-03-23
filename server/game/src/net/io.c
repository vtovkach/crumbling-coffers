#include <stdio.h>
#include <stdint.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>

#include "server-config.h" 

ssize_t udp_read(int target_fd)
{
    uint8_t temp_buf[UDP_DATAGRAM_SIZE];  

    ssize_t bytes = recvfrom(target_fd, temp_buf, sizeof(temp_buf), 0, NULL, NULL);
    if (bytes <= 0)
        return bytes;

    printf("%s\n", (char *) temp_buf);
    fflush(stdout);

    return bytes;
}

ssize_t udp_write()
{

    return 0;
}

ssize_t udp_read(int target_fd, struct sockaddr_in *addr, void *dest, size_t dest_size)
{
    if(!dest || !dest)
        return -1;
    
    struct iovec iov = {
        .iov_base = dest,
        .iov_len = dest_size
    };

    struct sockaddr_in src_addr;
    socklen_t addrlen = sizeof(src_addr);

    struct msghdr msg; 
    memset(&msg, 0, sizeof(msg));

    msg.msg_name = &src_addr; // Sender address 
    msg.msg_namelen = addrlen;
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;

    ssize_t bytes = recvmsg(target_fd, &msg, 0);
    if(bytes < 0)
        return -1;
    
    if(bytes < dest_size)
    {
        memset(dest, 0, dest_size);
        return 0;
    }

    if(msg.msg_flags & MSG_TRUNC)
    {
        memset(dest, 0, dest_size);
        return 0;
    }

    *addr = src_addr;

    return bytes;
}

ssize_t udp_write(int target_fd, struct sockaddr_in *addr, void *buf, size_t buf_size)
{
    if(!buf || !addr)
        return -1;

    ssize_t bytes = sendto(target_fd, buf, buf_size, 0, (struct sockaddr *) addr, sizeof(*addr));
    if(bytes != (ssize_t) buf_size)
        return -1;

    return bytes;
}