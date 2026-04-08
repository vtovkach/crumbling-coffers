#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "server-config.h"
#include "tcp_packets.h"
#include "test-config.h"

#define CONNECT_RETRIES    20
#define CONNECT_RETRY_US   50000   /* 50 ms between connect attempts */
#define POST_SEND_SLEEP_US 3000000  /* 500 ms after sending, before reading */
#define TEST_TIMEOUT_S     5       /* hard deadline for the entire test */
#define TEST_CLIENTS       10

static void on_timeout(int sig)
{
    (void)sig;
    fprintf(stderr, "FAIL: test timed out after %d seconds\n", TEST_TIMEOUT_S);
    exit(EXIT_FAILURE);
}

static void die(const char *msg)
{
    perror(msg);
    exit(EXIT_FAILURE);
}

static int connect_client(const char *ip)
{
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(atoi(SERVER_TCP_PORT));
    if (inet_pton(AF_INET, ip, &addr.sin_addr) != 1)
    {
        fprintf(stderr, "invalid IP address: %s\n", ip);
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < CONNECT_RETRIES; i++)
    {
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) die("socket");

        if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) == 0)
            return sock;

        close(sock);
        usleep(CONNECT_RETRY_US);
    }

    return -1;
}

static void print_result(int client_num, const struct OutgoingPacket *pkt)
{
    const char *result_str;
    if (pkt->event_type == SV_EVENT_MATCH_FOUND)
        result_str = "MATCH FOUND";
    else if (pkt->event_type == SV_EVENT_MATCH_NOT_FOUND)
        result_str = "MATCH NOT FOUND";
    else
        result_str = "UNKNOWN";

    printf("  Client %d:\n", client_num);
    printf("    Result:      %s (event_type=%u)\n", result_str, pkt->event_type);

    printf("    Player ID:   ");
    for (int i = 0; i < PLAYER_ID_SIZE; i++)
        printf("%02x", pkt->player_id[i]);
    printf("\n");

    printf("    Game ID:     ");
    for (int i = 0; i < GAME_ID_SIZE; i++)
        printf("%02x", pkt->game_id[i]);
    printf("\n");

    char ip_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &pkt->server_ip, ip_str, sizeof(ip_str));
    printf("    Server IP:   %s\n", ip_str);
    printf("    Server Port: %u\n", (unsigned)pkt->server_port);
}

int main(int argc, char *argv[])
{
    signal(SIGALRM, on_timeout);
    alarm(TEST_TIMEOUT_S);

    int         n_clients = TEST_CLIENTS;
    const char *ip        = IP_ADDRESS;

    if (argc >= 2)
    {
        n_clients = atoi(argv[1]);
        if (n_clients <= 0)
        {
            fprintf(stderr, "usage: %s [num_clients [ip]]\n", argv[0]);
            exit(EXIT_FAILURE);
        }
    }
    if (argc >= 3)
        ip = argv[2];

    int *socks = malloc((size_t)n_clients * sizeof(int));
    if (!socks) die("malloc");

    /* Connect all clients */
    for (int i = 0; i < n_clients; i++)
    {
        socks[i] = connect_client(ip);
        if (socks[i] < 0)
        {
            fprintf(stderr, "FAIL: client %d could not connect after %d attempts\n",
                    i, CONNECT_RETRIES);
            exit(EXIT_FAILURE);
        }
    }

    /* Each client sends a match request (event_type = 0) padded to TCP_SEGMENT_SIZE */
    uint8_t buf[TCP_SEGMENT_SIZE];
    memset(buf, 0, sizeof(buf));
    struct IncomingPacket req = { .event_type = SV_EVENT_MATCH_REQUEST };
    memcpy(buf, &req, sizeof(req));

    for (int i = 0; i < n_clients; i++)
    {
        ssize_t sent = send(socks[i], buf, sizeof(buf), 0);
        if (sent != (ssize_t)sizeof(buf))
            die("send");
    }

    /* Give the server time to process and respond */
    usleep(POST_SEND_SLEEP_US);

    /* Read and display the response from each client */
    int pass = 1;
    printf("Match results (%d players):\n", n_clients);

    for (int i = 0; i < n_clients; i++)
    {
        uint8_t rbuf[TCP_SEGMENT_SIZE];
        ssize_t n = recv(socks[i], rbuf, sizeof(rbuf), 0);

        if (n < (ssize_t)sizeof(struct OutgoingPacket))
        {
            fprintf(stderr, "FAIL: client %d received %zd bytes (expected >= %zu)\n",
                    i, n, sizeof(struct OutgoingPacket));
            pass = 0;
            continue;
        }

        struct OutgoingPacket pkt;
        memcpy(&pkt, rbuf, sizeof(pkt));
        print_result(i, &pkt);

        if (pkt.event_type != SV_EVENT_MATCH_FOUND &&
            pkt.event_type != SV_EVENT_MATCH_NOT_FOUND)
        {
            fprintf(stderr, "FAIL: client %d got unexpected event_type=%u\n",
                    i, pkt.event_type);
            pass = 0;
        }
    }

    /* Disconnect all clients */
    for (int i = 0; i < n_clients; i++)
    {
        shutdown(socks[i], SHUT_WR);
        char drain[64];
        while (recv(socks[i], drain, sizeof(drain), 0) > 0)
            ;
        close(socks[i]);
    }

    free(socks);

    if (pass)
        printf("PASS: all %d clients received a valid match response\n", n_clients);
    else
        printf("FAIL: one or more clients did not receive a valid match response\n");

    return pass ? 0 : 1;
}
