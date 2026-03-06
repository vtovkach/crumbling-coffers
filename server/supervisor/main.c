#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <sys/poll.h>

#include "util.h"
#include "server-config.h"

// Logging CONSTANTS  
#define LOG_DIR "log/"
#define LOG_ACTIVITY "log/supervisor"

volatile sig_atomic_t child_failure = 0;

void child_sig(int sig)
{
    (void)sig;
    child_failure = 1;
}

int main(void)
{
    if(signal(SIGUSR2, child_sig) == SIG_ERR)
    {
        perror("signal (supervisor)");
        return 1;
    }

    // Create log directory if it already does not exist 
    if(mkdir(LOG_DIR, 0755) == -1)
    {
        if(errno != EEXIST)
        {
            perror("mkdir (supervisor)");
            return 1;
        }
    }

    FILE *log_activity = fopen(LOG_ACTIVITY, "a");
    if(!log_activity)
    {
        perror("fopen (supervisor)");
        return 1;
    }

    // Get parent pid which will be passed to orchestrator  
    pid_t p_pid = getpid(); 

    // Create Orchestrator Process and save child pid 
    pid_t c_pid = fork(); 
    
    // Check if fork failed 
    if(c_pid < 0)
    {
        perror("fork (supervisor)");
        return 1;
    }
    
    // Here goes the child process 
    if(c_pid == 0)
    {
        // Make argument list to orchestrator process
        // Will additional necessary arguments later 

        char string_p_pid[32];
        snprintf(string_p_pid, sizeof(string_p_pid), "%d", p_pid);
        
        char *argv[] = {ORCHESTRATOR_PROCESS, string_p_pid, NULL};

        // Execute process 
        execv(ORCHESTRATOR_PROCESS, argv);

        // Runs only if execv fails 
        perror("execv (supervisor)");

        // Report error in a log file 
        char time[TIME_BUFFER_SIZE];
        getTime(time, TIME_BUFFER_SIZE);
        fprintf(log_activity, "%s: execv error: %s\n", time, strerror(errno));
        fflush(log_activity);

        _exit(1);
    }   

    char *userInput = NULL;
    size_t cap = 0;

    struct pollfd pfd = {.fd = STDIN_FILENO, .events = POLLIN};
    
    for(;;)
    {   
        int rc = poll(&pfd, 1, 2000);

        if(rc == 0)
        {
            // Timeout 
            // Something TODO here periodically 

            continue;
        }
        if(rc < 0)
        {
            perror("poll (supervisor)");
            kill(c_pid, SIGUSR1);
            break; 
        }

        // stdin is ready, now I can read input 
        ssize_t n = getline(&userInput, &cap, stdin);
        if(n == -1)
        {
            perror("getline (supervisor)");
            kill(c_pid, SIGUSR1);
            break;
        }

        printf("U: %s", userInput);

        if(strncmp(userInput, "exit", 4) == 0)
        {
            kill(c_pid, SIGUSR1);
            break;
        }
    }

    free(userInput);
    waitpid(c_pid, NULL, 0);

    return 0;
}