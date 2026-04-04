#include <stdatomic.h>
#include <unistd.h>
#include <stdio.h>

extern atomic_bool shutdown; 

void *matchmaker_run_t(void *args)
{   
    (void)args; // Silence compiler warning 

    while(!atomic_load(&shutdown))
    {
        printf("matchmaker\n");        
        sleep(1);
    }

    return NULL;
}