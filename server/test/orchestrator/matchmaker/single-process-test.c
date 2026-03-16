#include "test-config.h"

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

static ssize_t send_all(int fd, const void *buf, size_t len)
{
    const uint8_t *p = (const uint8_t *)buf;
    size_t total = 0;

    while (total < len) {
        ssize_t sent = send(fd, p + total, len - total, 0);

        if (sent < 0) {
            if (errno == EINTR) {
                continue;
            }
            return -1;
        }

        if (sent == 0) {
            break;
        }

        total += (size_t)sent;
    }

    return (ssize_t)total;
}

static ssize_t recv_all(int fd, void *buf, size_t len)
{
    uint8_t *p = (uint8_t *)buf;
    size_t total = 0;

    while (total < len) {
        ssize_t recvd = recv(fd, p + total, len - total, 0);

        if (recvd < 0) {
            if (errno == EINTR) {
                continue;
            }
            return -1;
        }

        if (recvd == 0) {
            break;
        }

        total += (size_t)recvd;
    }

    return (ssize_t)total;
}

static void bytes_to_hex_string(const uint8_t *src, size_t len, char *dst, size_t dst_size)
{
    size_t i;
    size_t pos = 0;

    if (dst == NULL || dst_size == 0) {
        return;
    }

    dst[0] = '\0';

    for (i = 0; i < len; ++i) {
        int written = snprintf(dst + pos, dst_size - pos, "%02X", src[i]);

        if (written < 0) {
            break;
        }

        if ((size_t)written >= (dst_size - pos)) {
            pos = dst_size - 1;
            break;
        }

        pos += (size_t)written;
    }

    dst[pos] = '\0';
}

static int connect_to_server(void)
{
    int sockfd;
    struct sockaddr_in server_addr;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        fprintf(stderr, "ERROR: socket() failed: %s\n", strerror(errno));
        return -1;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);

    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) != 1) {
        fprintf(stderr, "ERROR: invalid server IP: %s\n", SERVER_IP);
        close(sockfd);
        return -1;
    }

    printf("Connecting to server %s:%d...\n", SERVER_IP, SERVER_PORT);

    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        fprintf(stderr, "ERROR: connect() failed: %s\n", strerror(errno));
        close(sockfd);
        return -1;
    }

    printf("TCP connection established.\n");
    return sockfd;
}

int main(void)
{
    int sockfd = connect_to_server();
    if (sockfd < 0) {
        return TEST_CLIENT_FAILURE;
    }

    uint8_t send_buf[TEST_PACKET_SIZE];
    memset(send_buf, 0, sizeof(send_buf));

    size_t join_len = strlen(TEST_JOIN_MESSAGE);
    if (join_len > sizeof(send_buf)) {
        fprintf(stderr, "ERROR: TEST_JOIN_MESSAGE is larger than TEST_PACKET_SIZE\n");
        close(sockfd);
        return TEST_CLIENT_FAILURE;
    }

    memcpy(send_buf, TEST_JOIN_MESSAGE, join_len);

    printf("Sending join packet...\n");

    ssize_t sent = send_all(sockfd, send_buf, sizeof(send_buf));
    if (sent < 0 || (size_t)sent != sizeof(send_buf)) {
        fprintf(stderr, "ERROR: send_all() failed. sent=%zd errno=%s\n", sent, strerror(errno));
        close(sockfd);
        return TEST_CLIENT_FAILURE;
    }

    printf("Join packet sent successfully.\n");

    uint8_t ack_buf[TEST_PACKET_SIZE];
    memset(ack_buf, 0, sizeof(ack_buf));

    ssize_t recvd = recv_all(sockfd, ack_buf, sizeof(ack_buf));
    if (recvd < 0) {
        fprintf(stderr, "ERROR: recv_all() for ACK failed: %s\n", strerror(errno));
        close(sockfd);
        return TEST_CLIENT_FAILURE;
    }

    if ((size_t)recvd != sizeof(ack_buf)) {
        fprintf(stderr,
                "ERROR: ACK packet size mismatch. expected=%zu received=%zd\n",
                sizeof(ack_buf),
                recvd);
        close(sockfd);
        return TEST_CLIENT_FAILURE;
    }

    if (memcmp(ack_buf, TEST_ACK_TEXT, strlen(TEST_ACK_TEXT)) != 0) {
        char ack_preview[32];
        memset(ack_preview, 0, sizeof(ack_preview));
        memcpy(ack_preview, ack_buf, sizeof(ack_preview) - 1);

        fprintf(stderr,
                "ERROR: ACK text mismatch. expected prefix=\"%s\" got prefix=\"%s\"\n",
                TEST_ACK_TEXT,
                ack_preview);
        close(sockfd);
        return TEST_CLIENT_FAILURE;
    }

    printf("ACK validated successfully.\n");

    TCP_Game_Packet packet;
    memset(&packet, 0, sizeof(packet));

    recvd = recv_all(sockfd, &packet, sizeof(packet));
    if (recvd < 0) {
        fprintf(stderr, "ERROR: recv_all() for TCP_Game_Packet failed: %s\n", strerror(errno));
        close(sockfd);
        return TEST_CLIENT_FAILURE;
    }

    if ((size_t)recvd != sizeof(packet)) {
        fprintf(stderr,
                "ERROR: confirmation packet size mismatch. expected=%zu received=%zd\n",
                sizeof(packet),
                recvd);
        close(sockfd);
        return TEST_CLIENT_FAILURE;
    }

    struct in_addr addr;
    addr.s_addr = packet.ip;

    char ip_str[INET_ADDRSTRLEN];
    if (inet_ntop(AF_INET, &addr, ip_str, sizeof(ip_str)) == NULL) {
        snprintf(ip_str, sizeof(ip_str), "INVALID_IP");
    }

    uint16_t host_port = packet.port;

    char game_id_hex[(GAME_ID_SIZE * 2) + 1];
    char player_id_hex[(PLAYER_ID_SIZE * 2) + 1];

    bytes_to_hex_string(packet.game_id, GAME_ID_SIZE, game_id_hex, sizeof(game_id_hex));
    bytes_to_hex_string(packet.player_id, PLAYER_ID_SIZE, player_id_hex, sizeof(player_id_hex));

    printf("Received TCP_Game_Packet successfully:\n");
    printf("  ip        = %s\n", ip_str);
    printf("  port      = %u\n", host_port);
    printf("  game_id   = %s\n", game_id_hex);
    printf("  player_id = %s\n", player_id_hex);

    printf("\nType 'exit' and press Enter to terminate.\n");

    while (1) {
        struct pollfd pfds[2];
        pfds[0].fd = STDIN_FILENO;
        pfds[0].events = POLLIN;
        pfds[0].revents = 0;

        pfds[1].fd = sockfd;
        pfds[1].events = POLLIN | POLLHUP | POLLERR;
        pfds[1].revents = 0;

        int rc = poll(pfds, 2, -1);
        if (rc < 0) {
            if (errno == EINTR) {
                continue;
            }

            fprintf(stderr, "ERROR: poll() failed: %s\n", strerror(errno));
            break;
        }

        if (pfds[1].revents & (POLLHUP | POLLERR | POLLNVAL)) {
            printf("Server connection closed.\n");
            break;
        }

        if (pfds[1].revents & POLLIN) {
            char tmp[256];
            ssize_t n = recv(sockfd, tmp, sizeof(tmp), 0);

            if (n < 0) {
                fprintf(stderr, "ERROR: recv() failed: %s\n", strerror(errno));
                break;
            }

            if (n == 0) {
                printf("Server connection closed.\n");
                break;
            }

            printf("Received %zd extra byte(s) from server.\n", n);
        }

        if (pfds[0].revents & POLLIN) {
            char line[256];

            if (fgets(line, sizeof(line), stdin) == NULL) {
                printf("Input closed. Exiting.\n");
                break;
            }

            line[strcspn(line, "\n")] = '\0';

            if (strcmp(line, "exit") == 0) {
                printf("Exiting client.\n");
                break;
            }

            printf("Unknown command: %s\n", line);
            printf("Type 'exit' to terminate.\n");
        }
    }

    close(sockfd);
    return TEST_CLIENT_SUCCESS;
}