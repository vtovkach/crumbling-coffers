#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/poll.h>
#include <stdatomic.h>
#include <sys/eventfd.h>
#include <pthread.h>

#include "broker.h"
#include "orchestrator.h"
#include "matchmaker.h"
#include "util.h"
#include "server-config.h"

// Logging CONSTANTS  
#define LOG_DIR "log/"
#define SUPERVISOR_LOG "log/supervisor"

atomic_bool shutdown = false; 

static FILE *setup_log(void)
{
    // Create log directory if it already does not exist 
    if(mkdir(LOG_DIR, 0755) == -1)
    {
        if(errno != EEXIST)
        {
            perror("mkdir (supervisor)");
            return NULL;
        }
    }

    return fopen(SUPERVISOR_LOG, "a");
}

static void shutdown_controller()
{
    // TODO 
}

int main(void)
{
    // Declare and initialize variables 
    FILE *log = NULL;
    struct Broker *broker = NULL;
    int orch_eventfd = -1;
    int matchmaker_eventfd = -1;
    int rc = -1;

    // Set up logging file 
    log = setup_log();
    if(!log) return 1;

    // Setup broker structure 
    broker = init_broker();
    if(!broker) goto error;

    // Signals orchestrator if there is an available packet in the queue 
    orch_eventfd = eventfd(0, EFD_NONBLOCK);
    // Signals sessions manager if there is an available packet in the queue 
    matchmaker_eventfd = eventfd(0, EFD_NONBLOCK); 

    // Launch Orchestrator 
    struct OrchArgs orch_args = {
        .broker = broker, 
        .log_file = log, 
        .orch_eventfd = orch_eventfd, 
        .matchmaker_eventfd = matchmaker_eventfd
    };

    pthread_t orch_t;
    rc = pthread_create(&orch_t, NULL, orch_run_t, &orch_args);
    if(rc != 0)
    {
        // Handle potential errors
        goto error; 
    }
    
    // Launch SessionsManager 
    struct MatchmakerArgs matchmaker_args = {
        .broker = broker,
        .log_file = log,
        .orch_eventfd = orch_eventfd,
        .matchmaker_eventfd = matchmaker_eventfd
    };

    pthread_t matchmaker_t; 
    rc = pthread_create(&matchmaker_t, 
                            NULL, 
                            matchmaker_run_t, 
                            &matchmaker_args);
    if(rc != 0)
    {
        // Handle potential errors 
        goto error;  
    }

    shutdown_controller();

    pthread_join(orch_t, NULL);
    pthread_join(matchmaker_t, NULL);

    return 0;

error: 

    atomic_store(&shutdown, 1);

    destroy_broker(broker);
    if(orch_eventfd != -1) close(orch_eventfd);
    if(matchmaker_eventfd != -1) close(matchmaker_eventfd);
    fclose(log);
    
    pthread_join(orch_t, NULL);
    pthread_join(matchmaker_t, NULL);

    return 1; 
}