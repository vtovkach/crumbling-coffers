#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "../include/common/util.h"

#define LOG_FILE "log/error.txt"

int main(void)
{  
    if(redirect_stderr(LOG_FILE) < 0)
    {
        char time[TIME_BUFFER_SIZE];
        getTime(time, TIME_BUFFER_SIZE);
        fprintf(stderr, "%s 'redirect_stderr' failed: %s\n", time, strerror(errno));
        return -1;
    }

    // Create Orchestrator Process 
    pid_t orchestrator_process = fork(); 
    
    // Check if fork failed 
    if(orchestrator_process < 0)
    {
        char time[TIME_BUFFER_SIZE];
        getTime(time, TIME_BUFFER_SIZE);
        fprintf(stderr, "%s 'redirect_stderr' failed: %s\n", time, strerror(errno));
        return -1;
    }
    
    // Here goes the child process 
    if(orchestrator_process == 0)
    {
        // Spawn orchestrator 
        // TODO 

        // execve(...)

        _exit(0);
    }

    waitpid(orchestrator_process, NULL, 0);

    return 0;
}