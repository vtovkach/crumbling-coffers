#ifndef _CONFIG_H
#define _CONFIG_H

#define ORCH_LOG_PATH           "log/orchestrator"
#define SERVER_TCP_PORT         "10000"
#define MAX_TCP_QUEUE           128
#define ORCH_MAX_EPOLL_EVENTS   512
#define HASH_TABLE_SIZE         4096
#define TCP_SEGMENT_SIZE        20

// UDP GAME PROCESS 
#define UDP_GAME_PROCESS_PORT   "10001"
#define GAME_PROCESS            "./bin/game/game"
#define GM_MAX_EPOLL_EVENTS     64  
#define UDP_DATAGRAM_SIZE       20

#endif