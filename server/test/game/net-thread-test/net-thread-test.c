// udp_test_sender.c
// Sends packets for multiple players.
// For each player: send INIT, then after 100 ms send REGULAR.
// Each packet is exactly 200 bytes.
// No network byte order conversion is performed.

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define TARGET_IP   "127.0.0.1"
#define TARGET_PORT 5000

#define UDP_DATAGRAM_SIZE 200
#define GAME_ID_SIZE      16
#define PLAYER_ID_SIZE    16
#define PLAYERS_NUM       6

#define CTRL_REL_PACKET   0x01
#define CTRL_REG_PACKET   0x02
#define CTRL_INIT_PACKET  0x04

#define SEND_REGULAR_PACKETS 1

struct __attribute__((packed)) Header
{
    uint8_t  game_id[GAME_ID_SIZE];
    uint8_t  player_id[PLAYER_ID_SIZE];
    uint16_t control;
    uint16_t payload_size;
    uint32_t seq_num;
};

static void fill_demo_bytes(uint8_t *buf, size_t size, uint8_t start)
{
    for (size_t i = 0; i < size; i++)
        buf[i] = (uint8_t)(start + i);
}

static void sleep_ms(long ms)
{
    struct timespec ts;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (ms % 1000) * 1000000L;
    nanosleep(&ts, NULL);
}

static int send_player_packet(int sockfd,
                              const struct sockaddr_in *dest_addr,
                              const uint8_t *game_id,
                              const uint8_t *player_id,
                              uint16_t control,
                              uint32_t seq_num,
                              const char *text)
{
    uint8_t packet[UDP_DATAGRAM_SIZE];
    struct Header header;
    int payload_len;
    ssize_t sent;

    memset(packet, 0, sizeof(packet));
    memset(&header, 0, sizeof(header));

    memcpy(header.game_id, game_id, GAME_ID_SIZE);
    memcpy(header.player_id, player_id, PLAYER_ID_SIZE);
    header.control = control;

    payload_len = (int)strlen(text);
    if ((size_t)payload_len > UDP_DATAGRAM_SIZE - sizeof(header))
    {
        fprintf(stderr, "payload too large\n");
        return -1;
    }

    header.payload_size = (uint16_t)payload_len;
    header.seq_num = seq_num;

    memcpy(packet, &header, sizeof(header));
    memcpy(packet + sizeof(header), text, (size_t)payload_len);

    sent = sendto(sockfd,
                  packet,
                  sizeof(packet),
                  0,
                  (const struct sockaddr *)dest_addr,
                  sizeof(*dest_addr));

    if (sent < 0)
    {
        perror("sendto");
        return -1;
    }

    return 0;
}

int main(void)
{
    int sockfd;
    struct sockaddr_in dest_addr;
    uint8_t game_id[GAME_ID_SIZE];
    uint8_t player_ids[PLAYERS_NUM][PLAYER_ID_SIZE];

    if (sizeof(struct Header) > UDP_DATAGRAM_SIZE)
    {
        fprintf(stderr, "Header is larger than datagram size.\n");
        return 1;
    }

    fill_demo_bytes(game_id, GAME_ID_SIZE, 0x10);

    for (uint32_t i = 0; i < PLAYERS_NUM; i++)
    {
        fill_demo_bytes(
            player_ids[i],
            PLAYER_ID_SIZE,
            (uint8_t)(0x20 + (i * 0x10))
        );
    }

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
    {
        perror("socket");
        return 1;
    }

    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(TARGET_PORT);

    if (inet_pton(AF_INET, TARGET_IP, &dest_addr.sin_addr) != 1)
    {
        perror("inet_pton");
        close(sockfd);
        return 1;
    }

    for (uint32_t i = 0; i < PLAYERS_NUM; i++)
    {
        char init_payload[128];
        char reg_payload[128];

        snprintf(init_payload, sizeof(init_payload),
                 "PLAYER[%u] INIT PACKET", i);

        snprintf(reg_payload, sizeof(reg_payload),
                 "PLAYER[%u] REGULAR PACKET", i);

        if (send_player_packet(sockfd,
                               &dest_addr,
                               game_id,
                               player_ids[i],
                               CTRL_INIT_PACKET,
                               (i * 2) + 1,
                               init_payload) != 0)
        {
            close(sockfd);
            return 1;
        }

        printf("Sent INIT for player %u\n", i);

#if SEND_REGULAR_PACKETS
        sleep_ms(100);

        if (send_player_packet(sockfd,
                               &dest_addr,
                               game_id,
                               player_ids[i],
                               CTRL_REG_PACKET,
                               (i * 2) + 2,
                               reg_payload) != 0)
        {
            close(sockfd);
            return 1;
        }

        printf("Sent REGULAR for player %u\n", i);
#endif

        sleep_ms(100);
    }

    close(sockfd);
    return 0;
}