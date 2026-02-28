#include <signal.h>
#include <stdio.h>

#include "../include/common/signals.h"

static volatile sig_atomic_t g_terminate = 0;

static void handle_terminate(int sig)
{
    (void)sig; // silences compiler warning 
    g_terminate = 1;
}

int signals_install(int sig)
{
    if (signal(sig, handle_terminate) == SIG_ERR)
    {
        perror("signal");
        return -1;
    }
    
    return 0;
}

bool signals_should_terminate(void)
{
    return g_terminate != 0;
}