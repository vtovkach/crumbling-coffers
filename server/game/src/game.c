#include "game.h"

#include <string.h>

struct Game *create_game(uint8_t *game_id, uint16_t map_id, size_t players_num, FILE *log_file)
{
    return NULL;
}

void destroy_game(struct Game *game, FILE *log_file)
{
}

void add_player(struct Game *game, struct Player *player)
{
}

void update_game(struct Game *game)
{
}

void form_auth_packet(struct Game *game, uint32_t start_tick, uint32_t stop_tick, struct AuthPacket *dst)
{
    memset(dst, 0, UDP_DATAGRAM_SIZE);

    memcpy(dst->header.game_id, game->game_id, GAME_ID_SIZE);
    dst->header.control      = CTRL_FLAG_AUTH;
    dst->header.payload_size = UDP_DATAGRAM_PAYLOAD_SIZE;
    dst->header.seq_num      = game->game_tick;

    dst->start_tick = start_tick;
    dst->stop_tick  = stop_tick;
    dst->n          = (uint8_t)game->players_num;

    for (size_t i = 0; i < game->players_num; i++)
    {
        struct Player *p = &game->players[i];
        memcpy(dst->players[i].player_id, p->player_id, PLAYER_ID_SIZE);
        dst->players[i].pos_x = (int32_t)p->pos_x;
        dst->players[i].pos_y = (int32_t)p->pos_y;
        dst->players[i].vel_x = (int32_t)p->vel_x;
        dst->players[i].vel_y = (int32_t)p->vel_y;
        dst->players[i].score = p->score;
    }
}

void form_init_packet(struct Game *game, uint32_t start_tick, uint32_t stop_tick, struct InitPacket *dst)
{
    memset(dst, 0, UDP_DATAGRAM_SIZE);

    memcpy(dst->header.game_id, game->game_id, GAME_ID_SIZE);
    dst->header.control      = CTRL_FLAG_INIT;
    dst->header.payload_size = UDP_DATAGRAM_PAYLOAD_SIZE;
    dst->header.seq_num      = game->game_tick;

    dst->start_tick = start_tick;
    dst->stop_tick  = stop_tick;
    dst->n          = (uint8_t)game->players_num;

    for (size_t i = 0; i < game->players_num; i++)
    {
        struct Player *p = &game->players[i];
        memcpy(dst->players[i].player_id, p->player_id, PLAYER_ID_SIZE);
        dst->players[i].x = (int32_t)p->pos_x;
        dst->players[i].y = (int32_t)p->pos_y;
    }
}
