#include "game.h"
#include "log_system.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define MAP_FILE_PATH "game/maps/castle_dungeon.dat"

struct Game *create_game(uint8_t *game_id, uint16_t map_id, size_t players_num, FILE *log_file)
{
    struct Game *game = calloc(1, sizeof(struct Game));
    if (!game) return NULL;

    memcpy(game->game_id, game_id, GAME_ID_SIZE);
    game->map_id      = map_id;
    game->players_num = players_num;
    game->status      = NOT_READY;

    game->players = calloc(players_num, sizeof(struct Player *));
    if (!game->players)
    {
        free(game);
        return NULL;
    }

    FILE *map_file = fopen(MAP_FILE_PATH, "r");
    if (!map_file)
    {
        log_message(log_file, "[create_game] failed to open map file\n");
        free(game->players);
        free(game);
        return NULL;
    }

    for (int i = 0; i < INIT_POSITIONS_COUNT; i++)
    {
        if (fscanf(map_file, "%f %f",
                   &game->initial_positions[i].pos_x,
                   &game->initial_positions[i].pos_y) != 2)
        {
            log_message(log_file, "[create_game] failed to read all initial positions from map file\n");
            break;
        }
    }

    fclose(map_file);
    return game;
}

void destroy_game(struct Game *game, FILE *log_file)
{
    if (!game) return;

    for (size_t i = 0; i < game->players_num; i++)
        destroy_player(game->players[i], log_file);

    free(game->players);
    free(game);
}

void add_player(struct Game *game, struct Player *player)
{
    game->players[player->player_idx] = player;

    player->pos_x = game->initial_positions[player->player_idx].pos_x;
    player->pos_y = game->initial_positions[player->player_idx].pos_y;
}

void update_game(struct Game *game, const struct Packet *pkt)
{
    if (pkt->header.control != 0x0000) return;

    const struct ClientRegularPacket *reg = (const struct ClientRegularPacket *)pkt;

    for (size_t i = 0; i < game->players_num; i++)
    {
        if (!game->players[i]) continue;

        if (memcmp(game->players[i]->player_id, reg->header.player_id, PLAYER_ID_SIZE) == 0)
        {
            update_player(game->players[i], reg);
            return;
        }
    }
}

void update_game_tick(struct Game *game, uint32_t game_tick)
{
    game->game_tick = game_tick;
}

void update_game_status(struct Game *game, enum GameStatus status)
{
    game->status = status;
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
        struct Player *p = game->players[i];
        if (!p) continue;
        memcpy(dst->players[i].player_id, p->player_id, PLAYER_ID_SIZE);
        dst->players[i].pos_x = p->pos_x;
        dst->players[i].pos_y = p->pos_y;
        dst->players[i].vel_x = p->vel_x;
        dst->players[i].vel_y = p->vel_y;
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
        struct Player *p = game->players[i];
        if (!p) continue;
        memcpy(dst->players[i].player_id, p->player_id, PLAYER_ID_SIZE);
        dst->players[i].x = p->pos_x;
        dst->players[i].y = p->pos_y;
    }
}
