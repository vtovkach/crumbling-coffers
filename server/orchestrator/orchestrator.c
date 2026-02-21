#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>

volatile sig_atomic_t terminate = 0;

void set_terminate(int sig)
{
    (void)sig; // silences the compiler warning  
    terminate = 1;
}

int main(void)
{
    // Connect the signal to a handler routine to allow supervisor process gracefully terminate the orchestrator process 
    if(signal(SIGUSR1, set_terminate) == SIG_ERR)
    {
        perror("signal");
        return 1;
    }

    for(;;)
    {
        // Check if parent wants to terminate the process 
        if(terminate)
        {
            // Later gracefully terminate this process 
            break;
        }

        sleep(2);
    }

    return 0;
}