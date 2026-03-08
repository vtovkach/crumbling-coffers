#ifndef __ORCHESTRATOR_H
#define __ORCHESTRATOR_H

#include <stdio.h>

#include "ds/hashmap.h"
#include "orchestrator/queue/game_queue.h"
struct Orchestrator 
{
    pid_t parent_pid;
    int listen_fd;
    int epoll_fd;
    FILE *log_file;
    HashTable *clients;
    struct GameQueue *gq;
};

int orchestrator_run(pid_t parent_pid);

#endif 