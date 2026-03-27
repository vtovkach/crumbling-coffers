// udp_test_sender.c
// Sends packets for multiple players.
// For each player: send reliable INIT and wait for ACK,
// then after 100 ms send REGULAR.
// Each packet is exactly 200 bytes.
// No network byte order conversion is performed.

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>

#define TARGET_IP   "127.0.0.1"
#define TARGET_PORT 5000

#define UDP_DATAGRAM_SIZE 200
#define UDP_ACK_TIMEOUT_MS 1000

#define GAME_ID_SIZE      16
#define PLAYER_ID_SIZE    16
#define PLAYERS_NUM       6

#define CTRL_FLAG_RELIABLE  (1u << 0)
#define CTRL_FLAG_INIT      (1u << 1)
#define CTRL_FLAG_ACK       (1u << 2)

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
    size_t payload_len;
    ssize_t sent;

    memset(packet, 0, sizeof(packet));
    memset(&header, 0, sizeof(header));

    memcpy(header.game_id, game_id, GAME_ID_SIZE);
    memcpy(header.player_id, player_id, PLAYER_ID_SIZE);
    header.control = control;

    payload_len = strlen(text);
    if (payload_len > UDP_DATAGRAM_SIZE - sizeof(header))
    {
        fprintf(stderr, "payload too large\n");
        return -1;
    }

    header.payload_size = (uint16_t)payload_len;
    header.seq_num = seq_num;

    memcpy(packet, &header, sizeof(header));
    memcpy(packet + sizeof(header), text, payload_len);

    sent = sendto(sockfd,
                  packet,
                  sizeof(packet),
                  0,
                  (const struct sockaddr *)dest_addr,
                  sizeof(*dest_addr));

    if (sent != UDP_DATAGRAM_SIZE)
    {
        if (sent < 0)
            perror("sendto");
        else
            fprintf(stderr, "sendto sent unexpected byte count: %zd\n", sent);
        return -1;
    }

    return 0;
}

static int wait_for_ack(int sockfd,
                        const uint8_t *game_id,
                        const uint8_t *player_id,
                        uint32_t expected_seq_num,
                        long timeout_ms)
{
    fd_set readfds;
    struct timeval tv;

    for (;;)
    {
        FD_ZERO(&readfds);
        FD_SET(sockfd, &readfds);

        tv.tv_sec = timeout_ms / 1000;
        tv.tv_usec = (timeout_ms % 1000) * 1000;

        int sel = select(sockfd + 1, &readfds, NULL, NULL, &tv);
        if (sel < 0)
        {
            if (errno == EINTR)
                continue;

            perror("select");
            return -1;
        }

        if (sel == 0)
        {
            fprintf(stderr, "ACK timeout for seq=%u\n", expected_seq_num);
            return -1;
        }

        if (FD_ISSET(sockfd, &readfds))
        {
            uint8_t recv_buf[UDP_DATAGRAM_SIZE];
            struct sockaddr_in from_addr;
            socklen_t from_len = sizeof(from_addr);

            ssize_t recvd = recvfrom(sockfd,
                                     recv_buf,
                                     sizeof(recv_buf),
                                     0,
                                     (struct sockaddr *)&from_addr,
                                     &from_len);

            if (recvd < 0)
            {
                if (errno == EINTR)
                    continue;

                perror("recvfrom");
                return -1;
            }

            if (recvd != UDP_DATAGRAM_SIZE)
            {
                fprintf(stderr, "Received unexpected datagram size: %zd\n", recvd);
                continue;
            }

            struct Header header;
            memcpy(&header, recv_buf, sizeof(header));

            if ((header.control & CTRL_FLAG_ACK) == 0)
                continue;

            if (memcmp(header.game_id, game_id, GAME_ID_SIZE) != 0)
                continue;

            if (memcmp(header.player_id, player_id, PLAYER_ID_SIZE) != 0)
                continue;

            if (header.seq_num != expected_seq_num)
                continue;

            return 0;
        }
    }
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
        uint32_t init_seq = (i * 2) + 1;
        uint32_t reg_seq = (i * 2) + 2;

        snprintf(init_payload, sizeof(init_payload),
                 "PLAYER[%u] INIT PACKET", i);

        snprintf(reg_payload, sizeof(reg_payload),
                 "PLAYER[%u] REGULAR PACKET", i);

        if (send_player_packet(sockfd,
                               &dest_addr,
                               game_id,
                               player_ids[i],
                               CTRL_FLAG_INIT | CTRL_FLAG_RELIABLE,
                               init_seq,
                               init_payload) != 0)
        {
            close(sockfd);
            return 1;
        }

        printf("Sent RELIABLE INIT for player %u\n", i);

        if (wait_for_ack(sockfd,
                         game_id,
                         player_ids[i],
                         init_seq,
                         UDP_ACK_TIMEOUT_MS) != 0)
        {
            fprintf(stderr, "Failed to receive ACK for player %u init packet\n", i);
            close(sockfd);
            return 1;
        }

        printf("Received ACK for player %u init packet\n", i);

#if SEND_REGULAR_PACKETS
        sleep_ms(100);

        if (send_player_packet(sockfd,
                               &dest_addr,
                               game_id,
                               player_ids[i],
                               0,
                               reg_seq,
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