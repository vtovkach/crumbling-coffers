#ifndef PLAYER_H
#define PLAYER_H

#include <stdint.h>
#include <server-config.h>

struct Player
{
    uint64_t seq_num;
    uint8_t player_id[PLAYER_ID_SIZE];
    int fd;
};

#endif
