#ifndef __GAME_H
#define __GAME_H
#include <stdint.h>

struct NetThreadArgs
{
    uint16_t udp_port; 

    // More fields will be defined later 
    // ...
    
};

int runGame(uint16_t game_port);

#endif