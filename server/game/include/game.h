#ifndef _GAME_H
#define _GAME_H

#include "server-config.h"
#include "packet.h"
#include "ds/hashmap.h"

#include <stdint.h>
#include <stddef.h>

struct PlayerInfo
{
    uint8_t player_id[PLAYER_ID_SIZE];

    uint32_t pos_x; 
    uint32_t pos_y;
    uint32_t vel_x;
    uint32_t vel_y;

    size_t items_num;
    struct Item *items;

    uint32_t score; 
};

struct Item
{
    uint8_t item_id;

    uint32_t pos_x;
    uint32_t pos_y;
};

struct Game
{
    uint8_t game_id[GAME_ID_SIZE];
    uint16_t map_id;

    size_t players_num;
    struct PlayerInfo *players;

    size_t items_num;
    HashTable *items; 
};

struct Game *create_game(uin8_t *game_id, uin16_t map_id, size_t players_num, FILE *log_file);

void destroy_game(struct Game *game, FILE *log_file);

void add_player();

void update_player();

struct Packet *get_auth_packet(struct Game *game);

#endif