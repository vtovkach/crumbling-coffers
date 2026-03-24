#ifndef __NET_THREAD_H
#define __NET_THREAD_H

#include <stdint.h>

#include "post_office.h"
#include "herald.h"

struct NetArgs 
{
    uint8_t *game_id;
    uint8_t *players_ids; 
    uint16_t port; 

    struct PostOffice *post_office;
    struct Herald *herald;

    size_t players_num;
};

void *run_net_t(void *t_args);

#endif