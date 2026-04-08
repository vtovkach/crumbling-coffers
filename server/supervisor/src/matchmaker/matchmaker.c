#include <stdatomic.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <errno.h>

#include "server-config.h"
#include "log_system.h"
#include "broker.h"
#include "broker-config.h"
#include "matchmaker/matchmaker.h"
#include "matchmaker/players_queue.h"
#include "matchmaker/port_manager.h"

#define BASE_PORT 10001
#define EPOLL_TIMEOUT 1000

#define PUBLIC_IP_ADDRESS GAME_SERVER_IP

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

static void process_broker_msg( FILE *log_file,
                                struct Broker *broker,
                                struct PlayersQueue *q_players,
                                int orch_eventfd
                              )
{
    struct BrokerMsg msg;
    if(pop_data_sessions_man(broker, &msg, sizeof(msg)) < 0)
    {
        log_error(log_file, "[process_broker_msg] pop_data_sessions_man failed", 0);
        return;
    }

    if(msg.kind != BROKER_MSG_CLIENT_REQUEST)
    {
        log_error(log_file, "[process_broker_msg] unexpected BrokerMsgKind", 0);
        return;
    }

    uint8_t  event_type = msg.client_request.event_type;
    int32_t  fd         = msg.client_request.fd;
    uint8_t *client_id  = msg.client_request.client_id;

    if(event_type == SV_EVENT_MATCH_REQUEST)
    {
        struct Player p = { .fd = (int)fd };
        memcpy(p.player_id, client_id, PLAYER_ID_SIZE);
        if(pq_add_player(q_players, &p, true) < 0)
        {
            log_error(log_file, "[process_broker_msg] pq_add_player failed", 0);
            return;
        }
    }
    else if(event_type == SV_EVENT_MATCH_CANCEL)
    {
        if(pq_remove_player(q_players, (int)fd) < 0)
        {
            log_error(log_file, "[process_broker_msg] pq_remove_player: player not found", 0);
            return;
        }

        struct BrokerMsg resp;
        resp.kind                    = BROKER_MSG_MATCH_RESULT;
        resp.match_result.fd         = fd;
        resp.match_result.event_type = SV_EVENT_MATCH_NOT_FOUND;
        memcpy(resp.match_result.client_id, client_id, PLAYER_ID_SIZE);

        if(push_data_orch(broker, &resp, sizeof(resp)) < 0)
        {
            log_error(log_file, "[process_broker_msg] push_data_orch failed", 0);
            return;
        }

        uint64_t sig = 1;
        if(write(orch_eventfd, &sig, sizeof(sig)) < 0)
        {
            log_error(log_file, "[process_broker_msg] orch_eventfd write failed", errno);
            return;
        }
    }
    else
    {
        log_error(log_file, "[process_broker_msg] unknown event_type, dropping message", 0);
        return;
    }

    char hex_id[PLAYER_ID_SIZE * 2 + 1];
    for(int i = 0; i < PLAYER_ID_SIZE; i++)
        snprintf(hex_id + i * 2, 3, "%02x", client_id[i]);

    char log_msg[256];
    snprintf(log_msg, sizeof(log_msg),
        "[process_broker_msg] event_type=%u player_id=%s fd=%d",
        event_type, hex_id, (int)fd);
    log_message(log_file, log_msg);
}

static void form_match( FILE *log_file,
                        struct PortManager *ports_manager,
                        struct PlayersQueue *q_players,
                        struct Broker *broker,
                        int orch_eventfd
                      )
{
    // TODO: pm_borrow_port, pop PLAYERS_PER_MATCH players via pop_from_queue
    // TODO: fork/exec GAME_PROCESS with borrowed port
    // TODO: pm_register_port on success, pm_return_port on failure
    // TODO: push BROKER_MSG_MATCH_RESULT to q_orch, signal orch_eventfd


}

static void process_events( FILE *log_file,
                            int n_events,
                            struct epoll_event *events,
                            int matchmaker_eventfd,
                            int form_match_eventfd,
                            struct Broker *broker,
                            struct PlayersQueue *q_players,
                            struct PortManager *ports_manager,
                            int orch_eventfd
                          )
{
    for(int i = 0; i < n_events; i++)
    {
        int fd = events[i].data.fd;

        if(fd == matchmaker_eventfd)
        {
            uint64_t val;
            read(fd, &val, sizeof(val));
            process_broker_msg(log_file, broker, q_players, orch_eventfd);
            continue;
        }

        if(fd == form_match_eventfd)
        {
            uint64_t val;
            read(fd, &val, sizeof(val));
            form_match(log_file, ports_manager, q_players, broker, orch_eventfd);
            continue;
        }
    }
}

static void check_match_readiness(  FILE *log_file, 
                                    struct PortManager *pm, 
                                    struct PlayersQueue *pq, 
                                    int event_fd)
{
    // Issue form match event if game queue and port is ready 
    if(pm_is_port(pm) && pq_ready(pq, PLAYERS_PER_MATCH))
    {
        uint64_t sig = 1; 
        if(write(event_fd, &sig, sizeof(sig)) < 0)
        {
            log_error(
                log_file, 
                "[matchmaker_run_t] form_match_eventfd write failed", 
                errno
            );
        }
    }
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
        if(rc > 0) process_events(
            m_args->log_file,
            rc,
            e_events,
            m_args->matchmaker_eventfd,
            form_match_eventfd,
            m_args->broker,
            q_players,
            ports_manager,
            m_args->orch_eventfd
        );

        // Check match readiness 
        check_match_readiness(
            m_args->log_file, 
            ports_manager, 
            q_players, 
            form_match_eventfd
        );

        // Check the status of port manager 
        if(!pm_status(ports_manager))
        {
            log_message(m_args->log_file, 
                "[matchmaker_run_t] ports manager failure"
            );
            goto exit;
        }
    }

exit: 
    atomic_store(&t_shutdown, true);

    if(q_players) pq_destroy(q_players);
    if(ports_manager) pm_destroy(ports_manager);
    if(form_match_eventfd != -1) close(form_match_eventfd);
    if(efd != -1) close(efd);

    return NULL;
}