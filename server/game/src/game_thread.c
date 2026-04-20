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
            "[handle_init_packet] drop INIT packet unauthorized player"
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

static bool run_connection_loop(FILE *log_file,
                                struct Game *game,
                                struct PostOffice *post_office,
                                atomic_bool *game_stop,
                                atomic_bool *net_stop,
                                uint8_t *players_ids,
                                size_t players_num,
                                uint32_t *server_tick,
                                uint32_t *start_tick,
                                uint32_t *stop_tick)
{
    uint32_t players_connected = 0;

    while(!atomic_load(game_stop) && !atomic_load(net_stop))
    {
        update_game_tick(game, *server_tick);

        for (;;)
        {
            uint8_t udp_packet[UDP_DATAGRAM_SIZE];
            int ret = post_office_mail_drop_pop(post_office, udp_packet, UDP_DATAGRAM_SIZE);
            if(ret < 0) break;

            struct Packet *pkt = (struct Packet *)udp_packet;
            if(pkt->header.control & CTRL_FLAG_INIT)
            {
                handle_init_packet(
                    log_file, game, &players_connected,
                    players_num, players_ids,
                    (struct InitPacket *)udp_packet
                );
            }
        }

        if(players_connected == (uint32_t)players_num)
        {
            *start_tick = *server_tick + GAME_INIT_TICKS;
            *stop_tick  = *start_tick  + GAME_DURATION_TICKS;
            update_game_status(game, INIT);
            return true;
        }

        if(*server_tick >= CONNECTION_DEADLINE_TICKS)
        {
            log_message(log_file, "[run_connection_loop] connection deadline reached, terminating\n");
            return false;
        }

        (*server_tick)++;
        sleep_ms(TICK_RATE_MS);
    }

    return false;
}

static void run_game_loop(FILE *log_file,
                          struct Game *game,
                          struct PostOffice *post_office,
                          struct Herald *herald,
                          atomic_bool *game_stop,
                          atomic_bool *net_stop,
                          size_t players_num,
                          uint32_t *server_tick,
                          uint32_t start_tick,
                          uint32_t stop_tick)
{
    while(!atomic_load(game_stop) && !atomic_load(net_stop))
    {
        update_game_tick(game, *server_tick);

        if(game->status == INIT && *server_tick >= start_tick)
            update_game_status(game, STARTED);

        if(game->status == STARTED && *server_tick >= stop_tick)
        {
            update_game_status(game, FINISHED);
            break;
        }

        if(game->status == STARTED)
        {
            for(size_t i = 0; i < players_num; i++)
            {
                uint8_t udp_packet[UDP_DATAGRAM_SIZE];
                int ret = post_office_read(post_office, i, udp_packet, UDP_DATAGRAM_SIZE);
                if(ret != 0) continue;
                update_game(game, (struct Packet *)udp_packet);
            }
        }

        struct Packet pkt = {0};

        if(game->status == INIT)
            form_init_packet(game, start_tick, stop_tick, (struct InitPacket *)&pkt);
        else if(game->status == STARTED)
            form_auth_packet(game, start_tick, stop_tick, (struct AuthPacket *)&pkt);

        herald_write(herald, &pkt, UDP_DATAGRAM_SIZE);

        (*server_tick)++;
        sleep_ms(TICK_RATE_MS);
    }

    (void)log_file;
}

void *run_game_t(void *t_args)
{
    uint8_t *game_id      = ((struct GameArgs *)t_args)->game_id;
    uint8_t *players_ids  = ((struct GameArgs *)t_args)->players_ids;
    size_t   players_num  = ((struct GameArgs *)t_args)->players_num;

    struct PostOffice *post_office = ((struct GameArgs *)t_args)->post_office;
    struct Herald     *herald      = ((struct GameArgs *)t_args)->herald;

    atomic_bool *game_stop = ((struct GameArgs *)t_args)->game_stop_flag;
    atomic_bool *net_stop  = ((struct GameArgs *)t_args)->net_stop_flag;

    FILE *log_file = ((struct GameArgs *)t_args)->log_file;

    uint32_t server_tick = 0;
    uint32_t start_tick  = 0;
    uint32_t stop_tick   = 0;

    struct Game *game = create_game(game_id, 0, players_num, log_file);
    if(!game)
        goto cleanup;

    bool rs = run_connection_loop(log_file, game, post_office, game_stop, net_stop,
                            players_ids, players_num,
                            &server_tick, &start_tick, &stop_tick);

    if(!rs) goto cleanup;
    
    run_game_loop(log_file, game, post_office, herald, game_stop, net_stop,
                  players_num, &server_tick, start_tick, stop_tick);

cleanup:
    destroy_game(game, log_file);
    atomic_store(game_stop, true);
    atomic_store(net_stop, true);
    return 0;
}