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

struct OrchArgs
{
    FILE *log_file;
    int orch_eventfd; 
    int sessions_manager_eventfd;
};

void *orch_t_run(void *);

int orchestrator_run(pid_t parent_pid);



#endif 