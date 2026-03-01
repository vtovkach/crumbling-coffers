#ifndef CONN_H
#define CONN_H

#include <stdio.h>              
#include "common/hashmap.h"     
#include "orchestrator/state/client.h"
#include "server-config.h" 

int closeConnection(FILE *log_file, int epoll_fd, int target_fd, struct HashTable *active_clients);

int acceptConnections(FILE *log_file, int listen_fd, int epoll_fd, struct HashTable *active_clients);

#endif