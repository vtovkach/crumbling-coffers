#include <stdatomic.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <errno.h>

#include "server-config.h"
#include "log_system.h"
#include "matchmaker/matchmaker.h"
#include "matchmaker/players_queue.h"
#include "matchmaker/port_manager.h"

#define BASE_PORT 10001
#define EPOLL_TIMEOUT 1000

extern atomic_bool t_shutdown;

static inline int add_fd_epoll( FILE *log_file, 
                                int efd, 
                                int tfd, 
                                int events
                            )
{
    struct epoll_event ev = {
        .data.fd = tfd,
        .events = events,
    };

    int ret = epoll_ctl(efd, EPOLL_CTL_ADD, tfd, &ev);
    if(ret != 0)
    {
        log_error(
            log_file, 
            "[add_fd_epoll] epoll_ctl failed", 
            errno
        );

        return -1;
    }
    return 0;
}

static void process_events(FILE *log_file)
{

}

static void check_match_readiness(FILE *log_file)
{

}

/* 
    Events

    - Message from Broker Ready
        - if find match:
            add player to the queue 
        - if cancel match:
            remove plyaer from the queue 

    - Form Match        
        - form match pass message to broker

*/
void *matchmaker_run_t(void *args)
{   
    struct MatchmakerArgs *m_args = (struct MatchmakerArgs *)args;
    struct PlayersQueue *q_players = NULL;
    struct PortManager *ports_manager = NULL;
    struct epoll_event e_events[EPOLL_MAX_EVENTS]; 

    int efd = -1; 
    int form_match_eventfd = -1; 

    // Define pool of ports 
    uint16_t ports[PORTS_LIMIT];
    for(size_t i = 0; i < PORTS_LIMIT; i++)
    {
        ports[i] = (uint16_t) (BASE_PORT + i + 1);
    }

    // Initialize Matchmaker Structures
    ports_manager = pm_create(ports, PORTS_LIMIT, m_args->log_file);
    if(!ports_manager)
    {
        log_error(
            m_args->log_file, 
            "[matchmaker_run_t] pm_create failed!", 
            0
        );
        goto exit; 
    }
    
    q_players = pq_create(PLAYERS_QUEUE_SIZE, m_args->log_file);
    if(!q_players)
    {
        log_error(
            m_args->log_file, 
            "[matchmaker_run_t] pq_create failed!", 
            0
        );
        goto exit; 
    }

    form_match_eventfd = eventfd(0, EFD_NONBLOCK);
    if(form_match_eventfd < 0)
    {
        log_error(
            m_args->log_file, 
            "[matchmaker_run_t] eventfd failed", 
            errno
        );
        goto exit; 
    }

    // Setup epoll
    efd = epoll_create1(0);
    if(efd < 0)
    {
        log_error(
            m_args->log_file, 
            "[matchmaker_run_t] epoll_create1 failed", 
            errno
        );
        goto exit; 
    }
    
    int ret; 
    ret = add_fd_epoll(
        m_args->log_file, 
        efd, 
        m_args->matchmaker_eventfd, 
        EPOLLIN
    );

    if(ret < 0) goto exit; 

    ret = add_fd_epoll(
        m_args->log_file, 
        efd, 
        form_match_eventfd, 
        EPOLLIN
    );

    if(ret < 0) goto exit; 

    while(!atomic_load(&t_shutdown))
    {   
        int rc = epoll_wait(
            efd, 
            e_events, 
            EPOLL_MAX_EVENTS, 
            EPOLL_TIMEOUT
        );

        if(rc < 0 && errno != EINTR)
        {
            log_error(
                m_args->log_file, 
                "[orch_run_t] epoll_wait critical failure", 
                errno
            );
            goto exit;
        }

        // Process events 
        if(rc > 0) process_events(m_args->log_file);

        // Issue form match event if game queue and port is ready 
        if(pm_is_port(ports_manager) && pq_ready(q_players, PLAYERS_PER_MATCH))
        {
            uint64_t sig = 1; 
            if(write(form_match_eventfd, &sig, sizeof(sig)) < 0)
            {
                log_error(m_args->log_file, 
                    "[matchmaker_run_t] form_match_eventfd write failed", 
                    errno
                );
            }
        }

        // Check the status of port manager 
        if(!pm_status(ports_manager))
        {
            log_message(m_args->log_file, 
                "[matchmaker_run_t] ports manager failure"
            );
            goto exit;
        }

        sleep(1);
    }

exit: 
    atomic_store(&t_shutdown, true);

    if(q_players) pq_destroy(q_players);
    if(ports_manager) pm_destroy(ports_manager);
    if(form_match_eventfd != -1) close(form_match_eventfd);
    if(efd != -1) close(efd);

    return NULL;
}