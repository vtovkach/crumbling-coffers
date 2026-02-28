#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

int make_udp_server_socket(uint16_t port)
{
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if(fd < 0)
    {
        perror("[make_udp_server_socket] socket");
        return -1;
    } 

    int opt = 1;
    if(setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        perror("[make_udp_server_socket] setsockopt");
        close(fd);
        return -1;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if(bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) 
    {
        perror("[make_udp_server_socket] bind");
        close(fd);
        return -1; 
    }

    int fl = fcntl(fd, F_GETFL, 0);
    if(fl < 0 || fcntl(fd, F_SETFL, fl | O_NONBLOCK) < 0)
    {
        perror("[make_udp_server_socket] fcntl");
        close(fd);
        return -1;
    } 
    
    return fd;
}