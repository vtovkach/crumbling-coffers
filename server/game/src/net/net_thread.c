#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdatomic.h>
#include <unistd.h>
#include <errno.h>
#include <sys/epoll.h>
#include <sys/types.h>

#include "server-config.h"
#include "log_system.h"
#include "net/net_thread.h"
#include "net/player_registry.h"
#include "net/udp_socket.h"
#include "net/io.h"
#include "packet.h"

void *run_net_t(void *t_args)
{   
    uint8_t *game_id = ((struct NetArgs *) t_args)->game_id; 
    uint8_t *players_ids = ((struct NetArgs *) t_args)->players_ids;
    size_t players_num = ((struct NetArgs *) t_args)->players_num;
    uint16_t port = ((struct NetArgs *) t_args)->port;

    int udp_fd = -1;
    int epoll_fd = -1;

    FILE *log_file = ((struct NetArgs *) t_args)->log_file;

    // Shared stuctures 
    struct PostOffice *post_office = ((struct NetArgs *) t_args)->post_office;
    struct Herald *herald = ((struct NetArgs *) t_args)->herald;

    atomic_bool *game_stop = ((struct NetArgs *) t_args)->game_stop_flag;
    atomic_bool *net_stop = ((struct NetArgs *) t_args)->net_stop_flag;

    // Local structure 
    struct PlayersRegistry *players_reg = players_registry_create(players_num);
    if(!players_reg) 
    {
        log_error(log_file, "[run_net_t] epoll_create1 failed.", errno);
        goto exit;
    }

    udp_fd = open_udp_socket(port);
    if(udp_fd < 0)
    {
        log_error(log_file, "[run_net_t] failed to open udp socket.", 0);
        goto exit; 
    }

    epoll_fd = epoll_create1(0);
    struct epoll_event e_events[GM_MAX_EPOLL_EVENTS];
    if(epoll_fd < 0)
    {
        log_error(log_file, "[run_net_t] epoll_create1 failed.", errno);
        goto exit;
    }

    struct epoll_event ev = {
        .data.fd = udp_fd, 
        .events = EPOLLIN
    };

    if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, udp_fd, &ev) < 0)
    {
        log_error(log_file, "[run_net_t] epoll_ctl failed.", errno);
        goto exit; 
    }

    while(!atomic_load(game_stop) && !atomic_load(net_stop))
    {   
        printf("Net Thread\n");
        sleep(1);
    }

exit:
    if(epoll_fd > 0)    close(epoll_fd);
    if(udp_fd > 0)      close(udp_fd);  
    players_registry_destroy(players_reg);
    atomic_store(game_stop, true);
    atomic_store(net_stop, true);
    return NULL;
}