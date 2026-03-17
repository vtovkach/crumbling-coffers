#ifndef __ORCHESTRATOR_H
#define __ORCHESTRATOR_H

#include <stdio.h>

#include "ds/hashmap.h"
#include "orchestrator/matchmaker/game_queue.h"
#include "orchestrator/matchmaker/port_manager.h"

struct Orchestrator 
{
    pid_t parent_pid;
    int listen_fd;
    int epoll_fd;
    FILE *log_file;
    HashTable *clients;
    struct GameQueue *gq;
    struct PortManager *pm; 
};

int orchestrator_run(pid_t parent_pid);

#endif 