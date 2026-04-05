#ifndef _BROKER_CONFIG_
#define _BROKER_CONFIG_

#include <stdint.h>
#include "server-config.h"
#include "tcp_packets.h"

#define BROKER_QUEUE_CAPACITY 128

/*
 * Orchestrator -> matchmaker: a client event (match request or cancel) 
 */
struct ClientRequestMsg
{
    uint8_t client_id[PLAYER_ID_SIZE];
    int32_t fd;
    uint8_t event_type; /* SV_EVENT_MATCH_REQUEST | SV_EVENT_MATCH_CANCEL */
};

/*
 * Matchmaker -> orchestrator: outcome of a match attempt
 * game_id / server_ip / server_port are only valid when
 * event_type == SV_EVENT_MATCH_FOUND
 */
struct MatchResultMsg
{
    uint8_t  client_id[PLAYER_ID_SIZE];
    int32_t  fd;
    uint8_t  event_type; /* SV_EVENT_MATCH_FOUND | SV_EVENT_MATCH_NOT_FOUND */
    uint8_t  game_id[GAME_ID_SIZE];
    uint32_t server_ip;
    uint16_t server_port;
};

enum BrokerMsgKind
{
    BROKER_MSG_CLIENT_REQUEST = 0, /* orch -> matchmaker */
    BROKER_MSG_MATCH_RESULT   = 1, /* matchmaker -> orch  */
};

struct BrokerMsg
{
    enum BrokerMsgKind kind;
    union {
        struct ClientRequestMsg client_request;
        struct MatchResultMsg   match_result;
    };
};

#endif
