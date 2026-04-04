#include <stdatomic.h>
#include <unistd.h>
#include <stdio.h>


extern atomic_bool shutdown; 

void *orch_run_t(void *args)
{
    (void)args; // Silence compiler warning 

    while(!atomic_load(&shutdown))
    {
        printf("orch\n");       
        sleep(1);
    }

    return NULL;
}