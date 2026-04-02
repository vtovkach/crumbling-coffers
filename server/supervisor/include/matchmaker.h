#ifndef _MATCHMAKER_
#define _MATCHMAKER_

#include <stdio.h>

struct MatchmakerArgs
{
    struct Broker *broker; 

    int orch_eventfd; 
    int matchmaker_eventfd;

    FILE *log_file;
};

void *matchmaker_run_t(void *);

#endif