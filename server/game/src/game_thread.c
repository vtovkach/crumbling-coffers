#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdatomic.h>
#include <unistd.h>
#include <errno.h>
#include <sys/epoll.h>
#include <sys/types.h>

#include "server-config.h"
#include "game_thread.h"
#include "post_office.h"
#include "herald.h"
#include "packet.h"

static void display_udp_packet(const uint8_t *udp_packet)
{
    struct Header header;
    memcpy(&header, udp_packet, sizeof(header));

    // ---- Print Game ID ----
    printf("Game ID: ");
    for (size_t j = 0; j < GAME_ID_SIZE; j++)
        printf("%02x ", header.game_id[j]);
    printf("\n");

    // ---- Print Player ID ----
    printf("Player ID: ");
    for (size_t j = 0; j < PLAYER_ID_SIZE; j++)
        printf("%02x ", header.player_id[j]);
    printf("\n");

    // ---- Print Payload ----
    const char *payload = (const char *)(udp_packet + sizeof(header));

    printf("Payload: %.*s\n",
           header.payload_size,
           payload);

    printf("--------\n");
}

static void sleep_ms(long ms)
{
    struct timespec ts;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (ms % 1000) * 1000000L;
    nanosleep(&ts, NULL);
}

void *run_game_t(void *t_args)
{   
    uint8_t *game_id = ((struct GameArgs *) t_args)->game_id; 
    uint8_t *players_ids = ((struct GameArgs *) t_args)->players_ids;
    size_t players_num = ((struct GameArgs *) t_args)->players_num;

    struct PostOffice *post_office = ((struct GameArgs *) t_args)->post_office;
    struct Herald *herald = ((struct GameArgs *) t_args)->herald;

    atomic_bool *game_stop = ((struct GameArgs *) t_args)->game_stop_flag;
    atomic_bool *net_stop = ((struct GameArgs *) t_args)->net_stop_flag;

    FILE *log_file = ((struct GameArgs *) t_args)->log_file;

    // Silence the compiler warnings for now 
    (void)log_file;
    (void)players_ids;

    uint32_t server_tick = 0;

    while(!atomic_load(game_stop) && !atomic_load(net_stop))
    {   
        // Process all reliable packets 
        //      -- retrieve all packets from the mail drop 

        // Process all regular packets
        //      - retrieve valid packets from each mailbox 

        // Form Authoritive Packet 
        //      - place into herald  

        for (;;)
        {
            uint8_t udp_packet[UDP_DATAGRAM_SIZE];
            int ret = post_office_mail_drop_pop(
                post_office, 
                udp_packet, 
                UDP_DATAGRAM_SIZE
            );

            if(ret < 0) break; // no more packets 

            display_udp_packet(udp_packet);
        }

        for (size_t i = 0; i < players_num; i++)
        {
            uint8_t udp_packet[UDP_DATAGRAM_SIZE];

            int ret = post_office_read(
                post_office, 
                i, 
                udp_packet, 
                UDP_DATAGRAM_SIZE
            );

            if (ret != 0) continue;

            display_udp_packet(udp_packet);
        }

        /*
            Prepare and send authoritative
            packet to all clients 
        */
       
        uint8_t packet[UDP_DATAGRAM_SIZE];
        struct Header *outgoing_header = (struct Header *)packet;

        memset(packet, 0, UDP_DATAGRAM_SIZE);
        memcpy(outgoing_header->game_id, game_id, GAME_ID_SIZE);
        memset(outgoing_header->player_id, 1, PLAYER_ID_SIZE);
        outgoing_header->control = CTRL_FLAG_AUTH;
        outgoing_header->seq_num = server_tick; 
        outgoing_header->payload_size = 0;

        herald_write(herald, packet, UDP_DATAGRAM_SIZE);

        server_tick++;
        sleep_ms(10);
    }

    return 0;
}