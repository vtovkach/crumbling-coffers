#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
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

static void net_receive_packets(FILE *log_file, 
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

        int res = udp_read(fd, &incoming_addr, recv_data, UDP_DATAGRAM_SIZE);

        if(res < 0)  break;

        if(res != UDP_DATAGRAM_SIZE)
        {
            // Incorrect packet size
            continue;
        }

        // Retrieve header 
        struct Header header; 
        memcpy(&header, recv_data, UDP_DATAGRAM_HEADER_SIZE);

        if(memcmp(header.game_id, game_id, GAME_ID_SIZE) != 0)
        {
            // Drop packet 
            // GAME ID Does not match
            continue; 
        }

        if(header.control & CTRL_FLAG_INIT)
        {   
            int ret = players_registry_add_next(
                players_reg, 
                header.player_id, 
                incoming_addr
            );

            if(ret != 0)
            {
                log_message(
                    log_file, "[net_receive_packets] failed \
                    to insert a new player to the registry.\n"
                );
                continue;
            }
        }

        if(header.control & CTRL_FLAG_RELIABLE)
        {
            // Reliable packet  
            uint8_t ack_packet[UDP_DATAGRAM_SIZE];
            memset(ack_packet, 0, UDP_DATAGRAM_SIZE);

            struct Header *outgoing_header = (struct Header *)ack_packet;
            memcpy(outgoing_header->game_id, header.game_id, GAME_ID_SIZE);
            memcpy(outgoing_header->player_id, header.player_id, PLAYER_ID_SIZE);
            outgoing_header->control = CTRL_FLAG_ACK;
            outgoing_header->payload_size = 0;
            outgoing_header->seq_num = header.seq_num;

            udp_write(fd, &incoming_addr, ack_packet, UDP_DATAGRAM_SIZE);
        }

        uint8_t idx = 0;
        int ret = players_registry_get_index(players_reg, header.player_id, &idx);
        if(ret < 0)
        {
            // User id does not belong to this game process 
            // DROP PACKET 
            continue;
        }

        // Validate sequence number (skip for INIT — first packet from this player)
        uint32_t *seq_num = players_registry_seq_get_by_index(players_reg, idx);
        if(!seq_num || (!(header.control & CTRL_FLAG_INIT) && header.seq_num <= *seq_num))
        {
            // Sequence number does not exist or existing is bigger then incoming
            // Drop packet 
            continue;
        }

        if(header.control & CTRL_FLAG_RELIABLE)
        {
            // Place packet inside dropbox 
            post_office_mail_drop_push(po, recv_data);
        }
        else
        {
            // Place packet inside mailbox  
            post_office_write(po, (size_t)idx, recv_data, UDP_DATAGRAM_SIZE);
        }

        // Update seq number      
        *seq_num = header.seq_num;        
    }
}

static void net_broadcast_state(FILE *log_file, 
                                int fd,
                                struct PlayersRegistry *players_reg,
                                size_t players_num,
                                const uint8_t *players_ids,
                                uint8_t *packet,
                                size_t packet_size)
{
    (void) log_file; // Silence compiler warning 

    for(size_t i = 0; i < players_num; i++)
    {
        struct sockaddr_in *cur_addr = players_registry_get_addr(
            players_reg, 
            players_ids + (i * PLAYER_ID_SIZE)
        );

        if(!cur_addr) continue;

        udp_write(fd, cur_addr, packet, packet_size);
    }
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
    struct PlayersRegistry *players_reg = players_registry_create(
        players_num, 
        players_ids
    );
    if(!players_reg)
    {
        log_error(log_file, "[run_net_t] players_registry_create failed.", errno);
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
        if(herald_read(herald, server_packet, UDP_DATAGRAM_SIZE) == 0)
        {
            // Broadcast packet to all clients
            net_broadcast_state(
                log_file, 
                udp_fd, 
                players_reg, 
                players_num, 
                players_ids, 
                server_packet, 
                UDP_DATAGRAM_SIZE
            );
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
            net_receive_packets(
                log_file, 
                udp_fd, 
                game_id, 
                post_office, 
                players_reg, 
                players_num
            );
        }
    }

exit:
    if(efd > 0)    close(efd);
    if(udp_fd > 0)      close(udp_fd);  
    players_registry_destroy(players_reg);
    atomic_store(game_stop, true);
    atomic_store(net_stop, true);
    return NULL;
}