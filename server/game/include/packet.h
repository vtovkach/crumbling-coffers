#ifndef _PACKET_ 
#define _PACKET_

#include "server-config.h"

#include <stdint.h>
#include <stddef.h>

#define CTRL_FLAG_RELIABLE      (1 << 0)
#define CTRL_FLAG_INIT          (1 << 1)
#define CTRL_FLAG_ACK           (1 << 2)  
#define CTRL_FLAG_AUTH          (1 << 3)

#define CTRL_FLAG_HIT_EVENT     (1 << 10)
#define CTRL_FLAG_PICK_EVENT    (1 << 11)

/*
    Packet Header Layout (40 bytes)

    game_id (16 bytes):
        Unique identifier for the game session.
        Used to quickly filter out packets that do not belong to this session.

    player_id (16 bytes):
        Unique identifier for the sender (player).

    control (2 bytes):
        Bitfield for packet metadata and flags.

    payload_size (2 bytes):
        Size of the payload in bytes (excluding header).

    seq_num (4 bytes):
        Sequence number used for ordering and duplicate detection.
*/

struct __attribute__((packed)) Header
{
    uint8_t game_id[GAME_ID_SIZE];
    uint8_t player_id[PLAYER_ID_SIZE];
    uint16_t control;
    uint16_t payload_size;
    uint32_t seq_num;
};

/* ── Generic (used by network layer to receive any packet) ───────────────── */

struct __attribute__((packed)) Packet
{
    struct Header header;
    uint8_t       payload[UDP_DATAGRAM_PAYLOAD_SIZE];
};

/* ── Client → Server ──────────────────────────────────────────────────────── */

struct __attribute__((packed)) ClientInitPacket
{
    struct Header header;
};

struct __attribute__((packed)) ClientRegularPacket
{
    struct Header header;
    int32_t       pos_x;
    int32_t       pos_y;
    int32_t       vel_x;
    int32_t       vel_y;
    uint32_t      score;
};

/* ── Server → Client ──────────────────────────────────────────────────────── */

struct __attribute__((packed)) AuthPlayerRecord
{
    uint8_t  player_id[PLAYER_ID_SIZE];
    int32_t  pos_x;
    int32_t  pos_y;
    int32_t  vel_x;
    int32_t  vel_y;
    uint32_t score;
};

struct __attribute__((packed)) InitPlayerRecord
{
    uint8_t player_id[PLAYER_ID_SIZE];
    int32_t x;
    int32_t y;
};

struct __attribute__((packed)) AuthPacket
{
    struct Header           header;
    uint32_t                start_tick;
    uint32_t                stop_tick;
    uint8_t                 n;
    struct AuthPlayerRecord players[];
};

struct __attribute__((packed)) InitPacket
{
    struct Header           header;
    uint32_t                start_tick;
    uint32_t                stop_tick;
    uint8_t                 n;
    struct InitPlayerRecord players[];
};

#endif
