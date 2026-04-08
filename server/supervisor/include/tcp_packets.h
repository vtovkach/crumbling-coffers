#ifndef _SV_PACKET_H_
#define _SV_PACKET_H_

#include "server-config.h"

#include <stdint.h>

#define SV_EVENT_MATCH_REQUEST   0
#define SV_EVENT_MATCH_CANCEL    1
#define SV_EVENT_MATCH_FOUND     2
#define SV_EVENT_MATCH_NOT_FOUND 3

/*
 * Incoming TCP packet from client.
 * 4 bytes: request type code.
 *   0 = match request
 *   1 = match cancel
 */
struct __attribute__((packed)) IncomingPacket
{
    uint32_t event_type;
};

/*
 * Outgoing TCP packet from client.
 * 4 bytes:             Packet Type code (2 = match found, 3 = match not found)
 * GAME_ID_SIZE bytes:  Game ID as raw bytes
 * PLAYER_ID_SIZE bytes: Player ID as raw bytes
 * 4 bytes:             Game Server IP
 * 2 bytes:             Game Port
 */
struct __attribute__((packed)) OutgoingPacket
{
    uint32_t event_type;
    uint8_t game_id[GAME_ID_SIZE];
    uint8_t player_id[PLAYER_ID_SIZE];
    uint32_t server_ip;
    uint16_t server_port; 
};

#endif