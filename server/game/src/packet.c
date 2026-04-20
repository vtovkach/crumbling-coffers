#include "game.h"

#include <string.h>

void form_auth_packet(struct Game *game, uint32_t start_tick, uint32_t stop_tick, struct Packet *dst)
{
    memset(dst, 0, sizeof(struct Packet));

    struct AuthPacket *pkt = (struct AuthPacket *)dst;

    memcpy(pkt->header.game_id, game->game_id, GAME_ID_SIZE);
    pkt->header.control      = CTRL_FLAG_AUTH;
    pkt->header.payload_size = (uint16_t)((sizeof(struct AuthPacket) - sizeof(struct Header)) +
                                           game->players_num * sizeof(struct AuthPlayerRecord));

    pkt->start_tick = start_tick;
    pkt->stop_tick  = stop_tick;
    pkt->n          = (uint8_t)game->players_num;

    for (size_t i = 0; i < game->players_num; i++)
    {
        struct Player *p = &game->players[i];
        memcpy(pkt->players[i].player_id, p->player_id, PLAYER_ID_SIZE);
        pkt->players[i].pos_x = (int32_t)p->pos_x;
        pkt->players[i].pos_y = (int32_t)p->pos_y;
        pkt->players[i].vel_x = (int32_t)p->vel_x;
        pkt->players[i].vel_y = (int32_t)p->vel_y;
        pkt->players[i].score = p->score;
    }
}

void form_init_packet(struct Game *game, uint32_t start_tick, uint32_t stop_tick, struct Packet *dst)
{
    memset(dst, 0, sizeof(struct Packet));

    struct InitPacket *pkt = (struct InitPacket *)dst;

    memcpy(pkt->header.game_id, game->game_id, GAME_ID_SIZE);
    pkt->header.control      = CTRL_FLAG_INIT;
    pkt->header.payload_size = (uint16_t)((sizeof(struct InitPacket) - sizeof(struct Header)) +
                                           game->players_num * sizeof(struct InitPlayerRecord));

    pkt->start_tick = start_tick;
    pkt->stop_tick  = stop_tick;
    pkt->n          = (uint8_t)game->players_num;

    for (size_t i = 0; i < game->players_num; i++)
    {
        struct Player *p = &game->players[i];
        memcpy(pkt->players[i].player_id, p->player_id, PLAYER_ID_SIZE);
        pkt->players[i].x = (int32_t)p->pos_x;
        pkt->players[i].y = (int32_t)p->pos_y;
    }
}
