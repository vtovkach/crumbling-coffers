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
#include "log_system.h"
#include "game.h"
#include "player.h"
#include "game_thread.h"
#include "post_office.h"
#include "herald.h"
#include "packet.h"

#define TICK_RATE_MS              16
#define CONNECTION_DEADLINE_TICKS 1875   /* 30 s  — max wait for all players to connect */
#define GAME_INIT_TICKS           312    /* 5 s   — lobby/countdown before game starts  */
#define GAME_DURATION_TICKS       7500   /* 2 min — total game duration                 */

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

static void handle_init_packet( FILE *log_file, 
                                struct Game *game, 
                                uint32_t *players_connected,
                                size_t players_num,
                                uint8_t *players_ids, 
                                struct InitPacket *pkt
                            )
{
    bool valid_player = false;
    for(size_t i = 0; i < players_num; i++)
    {
        int rc = memcmp(
            players_ids + (i * PLAYER_ID_SIZE), 
            pkt->header.player_id, 
            PLAYER_ID_SIZE
        );

        if(rc == 0)
        {
            valid_player = true;
            break;
        }
    }

    if(!valid_player) 
    {
        log_message(
            log_file, 
            "[handle_init_packet] drop INIT packet nauthorized player"
        );
        return;
    }

    struct Player *player = create_player(
        pkt->header.player_id, 
        (uint8_t)*players_connected, 
        log_file
    );

    if(!player) return; 

    add_player(game, player);
    (*players_connected)++;
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


    uint32_t server_tick = 0;
    uint32_t players_connected = 0; 
    uint32_t start_tick = 0; 
    uint32_t stop_tick = 0;

    struct Game *game = create_game(game_id, 0, players_num, log_file);
    if(!game) 
    {
        atomic_store(game_stop, true);
        atomic_store(net_stop, true);
        return 0; 
    }

    while(!atomic_load(game_stop) && !atomic_load(net_stop))
    {
        update_game_tick(game, server_tick);

        // Drain reliable packets (mail drop)
        for (;;)
        {
            uint8_t udp_packet[UDP_DATAGRAM_SIZE];
            int ret = post_office_mail_drop_pop(
                post_office,
                udp_packet,
                UDP_DATAGRAM_SIZE
            );

            if(ret < 0) break;

            struct Packet *pkt = (struct Packet *)udp_packet;
            if(pkt->header.control & CTRL_FLAG_INIT)
            {
                handle_init_packet(
                    log_file,
                    game,
                    &players_connected,
                    players_num,
                    players_ids,
                    (struct InitPacket *)udp_packet
                );
            }
        }

        // All players connected — transition to INIT
        if(game->status == NOT_READY && players_connected == (uint32_t)players_num)
        {
            start_tick = server_tick + GAME_INIT_TICKS;
            stop_tick  = start_tick  + GAME_DURATION_TICKS;
            update_game_status(game, INIT);
        }

        if(game->status == NOT_READY)
        {
            if(server_tick >= CONNECTION_DEADLINE_TICKS)
            {
                log_message(log_file, "[run_game_t] connection deadline reached, terminating\n");
                break;
            }
            goto advance_tick;
        }

        // INIT → STARTED
        if(game->status == INIT && server_tick >= start_tick)
            update_game_status(game, STARTED);

        // STARTED → FINISHED
        if(game->status == STARTED && server_tick >= stop_tick)
            update_game_status(game, FINISHED);

        if(game->status == FINISHED)
            break;

        // Process regular packets only during active game
        if(game->status == STARTED)
        {
            for(size_t i = 0; i < players_num; i++)
            {
                uint8_t udp_packet[UDP_DATAGRAM_SIZE];

                int ret = post_office_read(
                    post_office,
                    i,
                    udp_packet,
                    UDP_DATAGRAM_SIZE
                );

                if(ret != 0) continue;

                update_game(game, (struct Packet *)udp_packet);
            }
        }

        // Form and broadcast authoritative packet
        struct Packet pkt = {0};

        if(game->status == INIT)
            form_init_packet(game, start_tick, stop_tick, (struct InitPacket *)&pkt);
        else if(game->status == STARTED)
            form_auth_packet(game, start_tick, stop_tick, (struct AuthPacket *)&pkt);

        herald_write(herald, &pkt, UDP_DATAGRAM_SIZE);

        advance_tick:
        server_tick++;
        sleep_ms(TICK_RATE_MS);
    }

    destroy_game(game, log_file);
    atomic_store(game_stop, true);
    atomic_store(net_stop, true);
    return 0;
}