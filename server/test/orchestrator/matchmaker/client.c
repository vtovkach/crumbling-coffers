#include "test-config.h"

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

static void log_line(FILE *logf, const char *fmt, ...)
{
    if (logf == NULL) {
        return;
    }

    time_t now = time(NULL);
    struct tm tm_now;
    localtime_r(&now, &tm_now);

    char timebuf[64];
    strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S", &tm_now);

    fprintf(logf, "[%s] ", timebuf);

    va_list args;
    va_start(args, fmt);
    vfprintf(logf, fmt, args);
    va_end(args);

    fputc('\n', logf);
    fflush(logf);
}

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

static FILE *open_client_log(int client_index, char *path_buf, size_t path_buf_size)
{
    int written = snprintf(
        path_buf,
        path_buf_size,
        "%s/client_%d_pid_%ld.log",
        TEST_LOG_DIR,
        client_index,
        (long)getpid()
    );

    if (written < 0 || (size_t)written >= path_buf_size) {
        return NULL;
    }

    return fopen(path_buf, "w");
}

static int connect_to_server(FILE *logf)
{
    int sockfd;
    struct sockaddr_in server_addr;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        log_line(logf, "ERROR: socket() failed: %s", strerror(errno));
        return -1;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);

    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) != 1) {
        log_line(logf, "ERROR: invalid server IP: %s", SERVER_IP);
        close(sockfd);
        return -1;
    }

    log_line(logf, "Connecting to server %s:%d", SERVER_IP, SERVER_PORT);

    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        log_line(logf, "ERROR: connect() failed: %s", strerror(errno));
        close(sockfd);
        return -1;
    }

    log_line(logf, "TCP connection established");
    return sockfd;
}

int run_test_client(int client_index)
{
    char log_path[512];
    FILE *logf = open_client_log(client_index, log_path, sizeof(log_path));

    if (logf == NULL) {
        fprintf(stderr,
                "client %d pid %ld: failed to create log file\n",
                client_index,
                (long)getpid());
        return TEST_CLIENT_FAILURE;
    }

    log_line(logf, "Client started");
    log_line(logf, "PID: %ld", (long)getpid());
    log_line(logf, "Client index: %d", client_index);
    log_line(logf, "Log file: %s", log_path);

    int sockfd = connect_to_server(logf);
    if (sockfd < 0) {
        fclose(logf);
        return TEST_CLIENT_FAILURE;
    }

    uint8_t send_buf[TEST_PACKET_SIZE];
    memset(send_buf, 0, sizeof(send_buf));

    size_t join_len = strlen(TEST_JOIN_MESSAGE);
    if (join_len > sizeof(send_buf)) {
        log_line(logf, "ERROR: TEST_JOIN_MESSAGE is larger than TEST_PACKET_SIZE");
        close(sockfd);
        fclose(logf);
        return TEST_CLIENT_FAILURE;
    }

    memcpy(send_buf, TEST_JOIN_MESSAGE, join_len);

    log_line(logf, "Sending %zu bytes to server", sizeof(send_buf));
    log_line(logf, "Join message text: \"%s\"", TEST_JOIN_MESSAGE);

    ssize_t sent = send_all(sockfd, send_buf, sizeof(send_buf));
    if (sent < 0 || (size_t)sent != sizeof(send_buf)) {
        log_line(logf, "ERROR: send_all() failed. sent=%zd errno=%s", sent, strerror(errno));
        close(sockfd);
        fclose(logf);
        return TEST_CLIENT_FAILURE;
    }

    log_line(logf, "Successfully sent full packet");

    uint8_t ack_buf[TEST_PACKET_SIZE];
    memset(ack_buf, 0, sizeof(ack_buf));

    ssize_t recvd = recv_all(sockfd, ack_buf, sizeof(ack_buf));
    if (recvd < 0) {
        log_line(logf, "ERROR: recv_all() for ACK failed: %s", strerror(errno));
        close(sockfd);
        fclose(logf);
        return TEST_CLIENT_FAILURE;
    }

    if ((size_t)recvd != sizeof(ack_buf)) {
        log_line(logf,
                 "ERROR: ACK packet size mismatch. expected=%zu received=%zd",
                 sizeof(ack_buf),
                 recvd);
        close(sockfd);
        fclose(logf);
        return TEST_CLIENT_FAILURE;
    }

    log_line(logf, "Received ACK packet of %zu bytes", sizeof(ack_buf));

    if (memcmp(ack_buf, TEST_ACK_TEXT, strlen(TEST_ACK_TEXT)) != 0) {
        char ack_preview[32];
        memset(ack_preview, 0, sizeof(ack_preview));
        memcpy(ack_preview, ack_buf, sizeof(ack_preview) - 1);

        log_line(logf,
                 "ERROR: ACK text mismatch. expected prefix=\"%s\" got prefix=\"%s\"",
                 TEST_ACK_TEXT,
                 ack_preview);
        close(sockfd);
        fclose(logf);
        return TEST_CLIENT_FAILURE;
    }

    log_line(logf, "ACK validated successfully");

    TCP_Game_Packet packet;
    memset(&packet, 0, sizeof(packet));

    recvd = recv_all(sockfd, &packet, sizeof(packet));
    if (recvd < 0) {
        log_line(logf, "ERROR: recv_all() for TCP_Game_Packet failed: %s", strerror(errno));
        close(sockfd);
        fclose(logf);
        return TEST_CLIENT_FAILURE;
    }

    if ((size_t)recvd != sizeof(packet)) {
        log_line(logf,
                 "ERROR: confirmation packet size mismatch. expected=%zu received=%zd",
                 sizeof(packet),
                 recvd);
        close(sockfd);
        fclose(logf);
        return TEST_CLIENT_FAILURE;
    }

    struct in_addr addr;
    addr.s_addr = packet.ip;

    char ip_str[INET_ADDRSTRLEN];
    if (inet_ntop(AF_INET, &addr, ip_str, sizeof(ip_str)) == NULL) {
        snprintf(ip_str, sizeof(ip_str), "INVALID_IP");
    }

    uint16_t host_port = ntohs(packet.port);

    char game_id_hex[(GAME_ID_SIZE * 2) + 1];
    char player_id_hex[(PLAYER_ID_SIZE * 2) + 1];

    bytes_to_hex_string(packet.game_id, GAME_ID_SIZE, game_id_hex, sizeof(game_id_hex));
    bytes_to_hex_string(packet.player_id, PLAYER_ID_SIZE, player_id_hex, sizeof(player_id_hex));

    log_line(logf, "Received TCP_Game_Packet successfully");
    log_line(logf, "Packet data:");
    log_line(logf, "  ip        = %s", ip_str);
    log_line(logf, "  port      = %u", host_port);
    log_line(logf, "  game_id   = %s", game_id_hex);
    log_line(logf, "  player_id = %s", player_id_hex);

    log_line(logf, "Closing client socket");
    close(sockfd);

    log_line(logf, "Client finished successfully");
    fclose(logf);

    return TEST_CLIENT_SUCCESS;
}