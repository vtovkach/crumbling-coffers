#ifndef CLIENT_H
#define CLIENT_H

#include <stdint.h>        
#include <stddef.h>        
#include <stdbool.h>       
#include <netinet/in.h>    

#include "server-config.h" 

struct Client
{
    int fd;
    struct sockaddr_in addr;

    uint8_t buffer[TCP_SEGMENT_SIZE];
    size_t buf_size;
    size_t cur_size;

    uint8_t game_queue_info[TCP_SEGMENT_SIZE];
    size_t game_q_size;
    size_t game_q_cur_size;

    bool game_q_ready;
    bool is_received;
    bool ACK_sent;
    bool game_info_sent;
};

#endif