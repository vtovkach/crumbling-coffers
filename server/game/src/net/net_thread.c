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

#define EPOLL_WAIT_TIMEOUT 100      // time in milliseconds

static int net_receive_packets(FILE *log_file, 
                               int fd, 
                               uint8_t *game_id, 
                               struct PostOffice *po, 
                               struct PlayersRegistry *players_reg, 
                               size_t players_num)
{
    for(size_t i = 0; i < players_num; i++)
    {
        uint8_t recv_data[UDP_DATAGRAM_SIZE];
        struct sockaddr_in incoming_addr; 
        
        if(udp_read(fd, &incoming_addr, recv_data, UDP_DATAGRAM_SIZE) == UDP_DATAGRAM_SIZE)
        {
            // TYPES Of Packets:
            //      Reliable (implement later)
            //      Regular 

            // Validate packet 
            // Validate userid 
            // Validate seq num
            // Place packet inside mailbox
            // Update seq number

            recv_data = (struct Header *) recv_data;
            

        }
    }

    return 0;
}

void *run_net_t(void *t_args)
{   
    uint8_t *game_id = ((struct NetArgs *) t_args)->game_id; 
    uint8_t *players_ids = ((struct NetArgs *) t_args)->players_ids;
    size_t players_num = ((struct NetArgs *) t_args)->players_num;
    uint16_t port = ((struct NetArgs *) t_args)->port;

    int udp_fd = -1;
    int efd = -1;

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

    efd = epoll_create1(0);
    struct epoll_event e_events[GM_MAX_EPOLL_EVENTS];
    if(efd < 0)
    {
        log_error(log_file, "[run_net_t] epoll_create1 failed.", errno);
        goto exit;
    }

    struct epoll_event ev = {
        .data.fd = udp_fd, 
        .events = EPOLLIN
    };

    if(epoll_ctl(efd, EPOLL_CTL_ADD, udp_fd, &ev) < 0)
    {
        log_error(log_file, "[run_net_t] epoll_ctl failed.", errno);
        goto exit; 
    }

    while(!atomic_load(game_stop) && !atomic_load(net_stop))
    {   
        // Check Herald 
        uint8_t server_packet[UDP_DATAGRAM_SIZE];
        if(herald_read(herald, &server_packet, UDP_DATAGRAM_SIZE) == 0)
        {
            // Broadcast packet to all clients 
        }

        int e_ret = epoll_wait(
            efd, 
            e_events, 
            GM_MAX_EPOLL_EVENTS, 
            EPOLL_WAIT_TIMEOUT
        );

        if(e_ret == -1)
        {
            // Error check errno 
            if(errno == EINTR) { continue; }

            log_error(
                log_file, 
                "[run_net_t] epoll_wait critical failure.", 
                errno
            );

            goto exit; 
        }

        if(e_ret > 0)
        {
            // TODO 
            // There are incoming UDP datagrams
            // Invoke correct routine  
        }
        
        printf("Net Thread\n");
    }

exit:
    if(efd > 0)    close(efd);
    if(udp_fd > 0)      close(udp_fd);  
    players_registry_destroy(players_reg);
    atomic_store(game_stop, true);
    atomic_store(net_stop, true);
    return NULL;
}