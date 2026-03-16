#ifndef TEST_CONFIG_H
#define TEST_CONFIG_H

#define _POSIX_C_SOURCE 200809L

#include <stdint.h>
#include <stddef.h>

/* =========================
 * Adjustable configuration
 * ========================= */
#define SERVER_IP              "127.0.0.1"
#define SERVER_PORT            10000

/* Number of client processes to spawn */
#define TEST_CLIENT_COUNT      10

/* Delay between spawning clients in milliseconds */
#define TEST_SPAWN_DELAY_MS    100

/* Directory where per-client logs are stored */
#define TEST_LOG_DIR           "./logs"

#define TEST_CLIENT_TIMEOUT_SECONDS 5

/*
 * Fixed TCP segment size for:
 *   1) client join message
 *   2) ACK packet from server
 *
 * Client sends exactly TEST_PACKET_SIZE bytes.
 * ACK must also be read as exactly TEST_PACKET_SIZE bytes.
 */
#define TEST_PACKET_SIZE       200

/* Message sent by each client, padded with zeros to TEST_PACKET_SIZE */
#define TEST_JOIN_MESSAGE      "Hello, I want to join the game."

/* Expected ACK text at the start of the ACK packet */
#define TEST_ACK_TEXT          "ACK"

/* Server confirmation packet sizes */
#define GAME_ID_SIZE           16
#define PLAYER_ID_SIZE         16

struct __attribute__((packed)) TCP_Game_Packet
{
    uint32_t ip;
    uint16_t port;
    uint8_t  game_id[GAME_ID_SIZE];
    uint8_t  player_id[PLAYER_ID_SIZE];
};

typedef struct TCP_Game_Packet TCP_Game_Packet;

/* Return codes for each child process */
#define TEST_CLIENT_SUCCESS    0
#define TEST_CLIENT_FAILURE    1

/* =========================
 * Client API
 * ========================= */
int run_test_client(int client_index);

#endif /* TEST_CONFIG_H */