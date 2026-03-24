#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdatomic.h>
#include <unistd.h>
#include <errno.h>
#include <sys/epoll.h>
#include <sys/types.h>

#include "net/udp_socket.h"
#include "net/io.h"
#include "net/net_thread.h"
#include "server-config.h"
#include "game.h"

atomic_bool game_stop; 

void *run_game_t(void *t_args)
{    
    int tick = 0;
    for(;;)
    {   
        sleep(1);

        if(tick++ == 2) break; 
    }

    return 0; 
}