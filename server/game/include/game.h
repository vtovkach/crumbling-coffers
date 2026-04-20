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

enum GameStatus
{
    NOT_READY,
    INIT,
    STARTED
};

struct Game
{
    uint8_t game_id[GAME_ID_SIZE];
    uint16_t map_id;

    size_t players_num;
    struct Player *players;

    size_t items_num;
    HashTable *items;

    uint32_t game_tick;
    enum GameStatus status; 
};

struct Game *create_game(uint8_t *game_id, uint16_t map_id, size_t players_num, FILE *log_file);

void destroy_game(struct Game *game, FILE *log_file);

void add_player(struct Game *game, struct Player *player);

void update_game(struct Game *game);

void form_auth_packet(struct Game *game, uint32_t start_tick, uint32_t stop_tick, struct AuthPacket *dst);

void form_init_packet(struct Game *game, uint32_t start_tick, uint32_t stop_tick, struct InitPacket *dst);

#endif