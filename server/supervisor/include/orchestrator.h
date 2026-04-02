#ifndef __ORCHESTRATOR_H
#define __ORCHESTRATOR_H

#include <stdio.h>

#include "broker.h"

struct OrchArgs
{
    struct Broker *broker; 

    int orch_eventfd; 
    int sessions_manager_eventfd;

    FILE *log_file;
};

void *orch_run_t(void *);

#endif 