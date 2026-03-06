#ifndef _IO_H
#define _IO_H

#include <stdio.h>           
#include "ds/hashmap.h" 

int receiveData(int epoll_fd, int target_fd, HashTable *const clients, FILE *const log_file);

int sendData(FILE *const log_file, int epoll_fd, int target_fd, HashTable *const clients);

#endif