#include <stdatomic.h>
#include <unistd.h>
#include <stdio.h>

#include "matchmaker/matchmaker.h"

extern atomic_bool t_shutdown;

void *matchmaker_run_t(void *args)
{   
    struct MatchmakerArgs *m_args = (struct MatchmakerArgs *)args;

    // Port Manager 
    // Helper thread to collect back ports 


    while(!atomic_load(&t_shutdown))
    {   
        sleep(1);
    }

    return NULL;
}