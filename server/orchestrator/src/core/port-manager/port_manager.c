#include "orchestrator/core/port-manager/port_manager.h"

#include <stdlib.h>
#include <unistd.h>

struct ReaperArgs
{
    struct PortManager *pm;
    FILE *log_file;
};

static void *reaper_thread(void *args)
{
    struct ReaperArgs *r_args = (struct ReaperArgs *)args;

    while(!r_args->pm->reaper_thread_stop)
    {
        sleep(5);
    }

    return NULL;
}

struct PortManager *initPortManager(FILE *const log_file)
{
    struct PortManager *pm = calloc(1, sizeof(*pm));
    if(!pm)
    {
        // Error happened 
        // TODO 

        return NULL;
    }

    pm->port_queue = q_init(QUEUE_CAPACITY, sizeof(uint16_t));
    if(!pm->port_queue)
    {
        // Error happened 
        // TODO

        return NULL;
    }

    if(pthread_mutex_init(&pm->ports_lock, NULL) != 0)
    {
        // Error 
        // TODO 

        return NULL;
    }

    struct ReaperArgs *args = malloc(sizeof(*args));
    if(!args)
    {
        // Error 
        // TODO 

        return NULL;
    }

    args->pm = pm; 
    args->log_file = log_file;

    pm->reaper_thread_active = true;
    pm->reaper_thread_stop = false; 
    pm->reaper_exit_status = 0;

    if(pthread_create(&pm->reaper_thread, NULL, reaper_thread, args) != 0)
    {
        // Error 
        // TODO 

        return NULL;
    }

    return pm;
}