#ifndef _PLAYER_H
#define _PLAYER_H

#include "server-config.h"
#include "item.h"

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>

struct Player
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

struct Player *create_player(uint8_t *player_id, FILE *log_file);

void update_player(struct Player *player);

void destroy_player(struct Player *player, FILE *log_file);

#endif
