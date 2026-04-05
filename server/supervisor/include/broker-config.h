#ifndef _BROKER_CONFIG_
#define _BROKER_CONFIG_

#include <stdint.h>
#include "server-config.h"

#define BROKER_QUEUE_CAPACITY 128

/*
 * Message pushed by orchestrator → matchmaker via broker.
 * Layout: client_id (PLAYER_ID_SIZE bytes) | fd (4 bytes) | event_type (1 byte)
 *   event_type 0 = match request
 *   event_type 1 = match cancel
 *   event_type 2 = match found 
 */
struct __attribute__((packed)) BrokerMsg
{
    uint8_t  client_id[PLAYER_ID_SIZE];
    int32_t  fd;
    uint8_t  event_type;
};

#endif
