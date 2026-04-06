#ifndef __ORCHESTRATOR_H
#define __ORCHESTRATOR_H

#include <stdio.h>

struct OrchArgs
{
    struct Broker *broker; 

    int orch_eventfd; 
    int matchmaker_eventfd;

    FILE *log_file;
};

void *orch_run_t(void *);

#endif 