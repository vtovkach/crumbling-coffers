#include <stdio.h>
#include <stdint.h>
#include <sys/socket.h>
#include <unistd.h>

#include "server-config.h" 

ssize_t udp_read(int target_fd)
{
    uint8_t temp_buf[UDP_DATAGRAM_SIZE];  

    ssize_t bytes = recvfrom(target_fd, temp_buf, sizeof(temp_buf), 0, NULL, NULL);
    if (bytes <= 0)
        return bytes;

    fwrite(temp_buf, 1, bytes, stdout);
    fflush(stdout);

    return bytes;
}

ssize_t udp_write()
{

    return 0;
}