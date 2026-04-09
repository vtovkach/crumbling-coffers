#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/poll.h>
#include <stdatomic.h>
#include <sys/eventfd.h>
#include <pthread.h>
#include <stdbool.h>
#include <signal.h>

#include "broker.h"
#include "orchestrator/orchestrator.h"
#include "matchmaker/matchmaker.h"
#include "log_system.h"

// Logging CONSTANTS
#define LOG_DIR "log/"
#define SUPERVISOR_LOG "log/supervisor"

atomic_bool t_shutdown = false;

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

static void shutdown_controller(FILE *log)
{
    struct pollfd pfd = {
        .fd = STDIN_FILENO,
        .events = POLLIN
    };

    char *userInput = NULL;
    size_t cap = 0;

    for(;;)
    {
        int rc = poll(&pfd, 1, 2500);
        if(rc < 0)
        {
            if(errno == EINTR) continue;
            log_error(log, "[shutdown_controller] poll failed", errno);
            goto exit;
        }

        if(rc == 0)
        {
            if(atomic_load(&t_shutdown))
            {
                log_message(log,
                    "[shutdown_controller] shutdown signal received,"
                    " terminating");
                goto exit;
            }
            continue;
        }

        if(!(pfd.revents & POLLIN))
        {
            log_message(log,
                "[shutdown_controller] unexpected poll event on stdin,"
                " exiting");
            goto exit;
        }

        // Get user input
        ssize_t n = getline(&userInput, &cap, stdin);
        if(n == -1)
        {
            log_error(log, "[shutdown_controller] getline failed", errno);
            goto exit;
        }

        printf("U: %s", userInput);

        if(strncmp(userInput, "exit", 4) == 0)
        {
            log_message(log,
                "[shutdown_controller] user requested termination");
            goto exit;
        }
    }

exit:
    free(userInput);
}

int main(void)
{
    // Declare and initialize variables
    FILE *log = NULL;
    struct Broker *broker = NULL;
    int orch_eventfd = -1;
    int matchmaker_eventfd = -1;
    int rc = -1;
    bool orch_started = false;
    bool matchmaker_started = false;

    // Block SIGCHLD before spawning any threads so all threads inherit
    // the mask. The reaper thread in PortManager handles it exclusively
    // via sigtimedwait.
    sigset_t chld_set;
    sigemptyset(&chld_set);
    sigaddset(&chld_set, SIGCHLD);
    if(pthread_sigmask(SIG_BLOCK, &chld_set, NULL) != 0)
    {
        perror("[main] pthread_sigmask failed");
        return 1;
    }

    // Set up logging file
    log = setup_log();
    if(!log) return 1;

    // Setup broker structure
    broker = init_broker();
    if(!broker)
    {
        log_error(log, "[main] init_broker failed", 0);
        goto exit;
    }

    // Signals orchestrator if there is an available packet in the queue
    orch_eventfd = eventfd(0, EFD_NONBLOCK | EFD_SEMAPHORE);
    if(orch_eventfd == -1)
    {
        log_error(log, "[main] eventfd failed", errno);
        goto exit;
    }

    // Signals sessions manager if there is an available packet in the queue
    matchmaker_eventfd = eventfd(0, EFD_NONBLOCK | EFD_SEMAPHORE);
    if(matchmaker_eventfd == -1)
    {
        log_error(log, "[main] eventfd failed", errno);
        goto exit;
    }

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
        log_error(log, "[main] failed to create orchestrator thread", rc);
        goto exit;
    }
    orch_started = true;

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
        log_error(log, "[main] failed to create matchmaker thread", rc);
        goto exit;
    }
    matchmaker_started = true;

    shutdown_controller(log);

exit:

    atomic_store(&t_shutdown, true);

    destroy_broker(broker);
    if(orch_eventfd != -1) close(orch_eventfd);
    if(matchmaker_eventfd != -1) close(matchmaker_eventfd);
    if(log) fclose(log);

    if(orch_started) pthread_join(orch_t, NULL);
    if(matchmaker_started) pthread_join(matchmaker_t, NULL);

    return 0;
}