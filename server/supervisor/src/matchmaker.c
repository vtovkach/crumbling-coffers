#include <stdio.h>
#include <stddef.h>
#include <stdatomic.h>

extern atomic_bool shutdown; 

void *matchmaker_run_t(void *args)
{   
    (void)args; // Silence compiler warning 

    return NULL;
}