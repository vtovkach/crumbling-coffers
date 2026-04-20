#ifndef _GAME_H
#define _GAME_H

#include "server-config.h"
#include "packet.h"
#include "player.h"
#include "item.h"
#include "ds/hashmap.h"

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdbool.h>

#define INIT_POSITIONS_COUNT 7

enum GameStatus
{
    NOT_READY,
    INIT,
    STARTED,
    FINISHED
};

struct InitPos
{
    float pos_x;
    float pos_y;
};

struct Game
{
    uint8_t game_id[GAME_ID_SIZE];
    uint32_t game_tick;
    enum GameStatus status;
    uint16_t map_id;
    struct InitPos initial_positions[INIT_POSITIONS_COUNT];

    size_t players_num;
    struct Player **players;

    size_t items_num;
    HashTable *items;
};

struct Game *create_game(uint8_t *game_id, uint16_t map_id, size_t players_num, FILE *log_file);

void destroy_game(struct Game *game, FILE *log_file);

void add_player(struct Game *game, struct Player *player);

void update_game(struct Game *game, const struct Packet *pkt);

void update_game_tick(struct Game *game, uint32_t game_tick);

void update_game_status(struct Game *game, enum GameStatus status);

void form_auth_packet(struct Game *game, uint32_t start_tick, uint32_t stop_tick, struct AuthPacket *dst);

void form_init_packet(struct Game *game, uint32_t start_tick, uint32_t stop_tick, struct InitPacket *dst);

#endif