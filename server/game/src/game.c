#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdatomic.h>
#include <unistd.h>
#include <errno.h>
#include <sys/epoll.h>
#include <sys/types.h>

#include "server-config.h"
#include "game.h"
#include "post_office.h"
#include "herald.h"

void *run_game_t(void *t_args)
{   
    uint8_t *game_id = ((struct GameArgs *) t_args)->game_id; 
    uint8_t *players_ids = ((struct GameArgs *) t_args)->players_ids;
    size_t players_nun = ((struct GameArgs *) t_args)->players_num;

    struct PostOffice *post_office = ((struct GameArgs *) t_args)->post_office;
    struct Herald *herald = ((struct GameArgs *) t_args)->herald;

    atomic_bool *game_stop = ((struct GameArgs *) t_args)->game_stop_flag;
    atomic_bool *net_stop = ((struct GameArgs *) t_args)->net_stop_flag;

    FILE *log_file = ((struct GameArgs *) t_args)->log_file;

    while(!atomic_load(game_stop) && !atomic_load(net_stop))
    {   
        printf("Game Thread\n");
        sleep(1);
    }

    return 0; 
}