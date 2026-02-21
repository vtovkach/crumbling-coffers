#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "../include/common/util.h"

// Logging CONSTANTS  
#define LOG_DIR "log/"
#define LOG_ACTIVITY "log/supervisor"

// Path to orchestrator process executable 
#define ORCHESTRATOR_PROCESS "bin/orchestrator/orchestrator"

int main(void)
{
    // Create log directory if it already does not exist 
    if(mkdir(LOG_DIR, 0755) == -1)
    {
        if(errno != EEXIST)
        {
            perror("mkdir");
            return -1;
        }
    }

    FILE *log_activity = fopen(LOG_ACTIVITY, "a");
    if(!log_activity)
    {
        perror("fopen");
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