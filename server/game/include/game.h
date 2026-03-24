#ifndef __GAME_H
#define __GAME_H
#include <stdint.h>

#include "herald.h"
#include "post_office.h"
#include "server-config.h" 

struct GameArgs
{
    uint8_t *game_id;
    uint8_t *players_ids; 

    struct PostOffice *post_office; 
    struct Herald *herald;

    size_t players_num;
};

void *run_game_t(void *t_args);

#endif