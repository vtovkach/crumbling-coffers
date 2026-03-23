#ifndef _IO_
#define _IO_

#include <stddef.h>
#include <sys/socket.h>
#include <netinet/in.h>

ssize_t udp_read(int target_fd, struct sockaddr_in *addr, void *dest, size_t dest_size);

ssize_t udp_write(int target_fd, struct sockaddr_in *addr, void *buf, size_t buf_size);

#endif