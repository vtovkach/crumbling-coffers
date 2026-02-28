#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <signal.h>

#include "orchestrator/orchestrator.h"

int main(int argc, char *argv[])
{
    if(argc != 2)
    {
        printf("orchestrator: incorrect argument list.\n");
        return 1;
    }
    
    // Obtain parent pid 
    pid_t p_pid = (pid_t)atoi(argv[1]);

    int status = orchestrator_run(p_pid);

    // Let parent know that child terminated 
    kill(p_pid, SIGUSR2);

    return status; 
}