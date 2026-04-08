#ifndef _CONFIG_H
#define _CONFIG_H

// Orchestrator Process 
#define ORCHESTRATOR_PROCESS    "./bin/orch"
#define ORCH_LOG_PATH           "log/orchestrator"
#define SERVER_TCP_PORT         "10000" // must be smaller then (65535 - PORTS_LIMIT) to avoid overflow in port manager
#define PORTS_LIMIT             20 
#define PLAYERS_QUEUE_SIZE      100
#define MAX_TCP_QUEUE           128
#define MAX_GAME_QUEUE          128
#define EPOLL_MAX_EVENTS        512
#define EPOLL_TIMEOUT           1000
#define HASH_TABLE_SIZE         4096
#define TCP_SEGMENT_SIZE        200
#define PLAYERS_PER_MATCH 2

// UDP GAME PROCESS 
#define GAME_SERVER_IP          "127.0.0.1"
#define UDP_GAME_PROCESS_PORT   "10001"
#define GAME_PROCESS            "./bin/game"
#define GM_MAX_EPOLL_EVENTS     64  

// UDP DATAGRAM CONSTANTS 
#define UDP_DATAGRAM_SIZE           200
#define UDP_DATAGRAM_HEADER_SIZE    40
#define UDP_DATAGRAM_PAYLOAD_SIZE   160

#define GAME_ID_SIZE 16
#define PLAYER_ID_SIZE 16

#endif