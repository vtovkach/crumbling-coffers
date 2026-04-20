#ifndef _PLAYER_H
#define _PLAYER_H

#include "server-config.h"
#include "item.h"
<<<<<<< HEAD
#include "packet.h"
=======
>>>>>>> 4ff7ed2 (Squash in PROJ-151-create-packetization-utility-for-game-server (pull request #70))

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>

struct Player
{
    uint8_t player_id[PLAYER_ID_SIZE];
<<<<<<< HEAD
    uint8_t player_idx; 
    
    float pos_x;
    float pos_y;
    float vel_x;
    float vel_y;
=======

    uint32_t pos_x;
    uint32_t pos_y;
    uint32_t vel_x;
    uint32_t vel_y;
>>>>>>> 4ff7ed2 (Squash in PROJ-151-create-packetization-utility-for-game-server (pull request #70))

    size_t items_num;
    struct Item *items;

    uint32_t score;
};

<<<<<<< HEAD
struct Player *create_player(uint8_t *player_id, uint8_t player_idx, FILE *log_file);

void update_player(struct Player *player, const struct ClientRegularPacket *pkt);
=======
struct Player *create_player(uint8_t *player_id, FILE *log_file);

void update_player(struct Player *player);
>>>>>>> 4ff7ed2 (Squash in PROJ-151-create-packetization-utility-for-game-server (pull request #70))

void destroy_player(struct Player *player, FILE *log_file);

#endif
