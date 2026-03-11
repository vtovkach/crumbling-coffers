#ifndef CLIENT_H
#define CLIENT_H

#include <stdint.h>        
#include <stddef.h>        
#include <stdbool.h>       
#include <netinet/in.h>    
#include <time.h>

#include "server-config.h" 

struct Client
{
    uint64_t client_id; 

    int fd;
    struct sockaddr_in addr;

    uint8_t buffer[TCP_SEGMENT_SIZE];
    size_t buf_size;
    size_t cur_size;

    uint8_t game_queue_info[TCP_SEGMENT_SIZE];
    size_t game_q_size;
    size_t game_q_cur_size;

    bool game_q_ready;   // Indicates whether the buffer containing game connection information is ready to be sent
    bool is_received;    // Indicates whether the client has sent all initialization data
    bool ACK_sent;       // Indicates whether the acknowledgment message was sent to the client 
    bool game_info_sent; // Indicates whether game information was sent to the client

    struct timespec ts;
};

#endif