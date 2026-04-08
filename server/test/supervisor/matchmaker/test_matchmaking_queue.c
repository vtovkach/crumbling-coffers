/*
 * test_matchmaking_queue.c
 *
 * Integration test: verifies that players queued when no ports are
 * available are matched once ports become free, including when stages
 * arrive with uneven client counts so players from different stages
 * pair with each other.
 *
 * Phase 1 — fill the port pool:
 *   Connect (PORTS_LIMIT * PLAYERS_PER_MATCH) clients.  This forms
 *   PORTS_LIMIT simultaneous games and exhausts every available port.
 *   Verify all phase-1 clients receive MATCH_FOUND.
 *
 * Queue stages (2–5) — arrive while pool is exhausted:
 *   Each stage connects a deliberately uneven number of clients so that
 *   lone players from one stage pair with players from the next.  All
 *   requests queue with no port available.
 *
 *   Stage 2:  7 clients  (odd  — 1 player left waiting for a partner)
 *   Stage 3:  5 clients  (odd  — lone from stage 2 pairs with stage 3)
 *   Stage 4:  9 clients  (odd  — 1 player left waiting again)
 *   Stage 5:  3 clients  (odd  — lone from stage 4 pairs with stage 5)
 *   Total queued: 24 clients = 12 matches
 *
 *   Once the phase-1 game processes exit (~5 s) the reaper returns all
 *   20 ports and the matchmaker forms all 12 pending matches in a burst.
 *   Verify every queued client receives MATCH_FOUND.
 *
 * Requires: PORTS_LIMIT=20, PLAYERS_PER_MATCH=2, game lifetime ~5 s.
 * Total runtime: ~12 s.  Hard deadline: TEST_TIMEOUT_S.
 */

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

/* Phase 1: enough to spawn one game per port and exhaust the pool */
#define PHASE1_CLIENTS  (PORTS_LIMIT * PLAYERS_PER_MATCH)

/* Queue stages — odd counts so players from adjacent stages pair up */
#define STAGE2_CLIENTS   7
#define STAGE3_CLIENTS   5
#define STAGE4_CLIENTS   9
#define STAGE5_CLIENTS   3
#define TOTAL_QUEUED    (STAGE2_CLIENTS + STAGE3_CLIENTS + \
                         STAGE4_CLIENTS + STAGE5_CLIENTS)  /* must be even */

#define CONNECT_RETRIES     20
#define CONNECT_RETRY_US    50000    /* 50 ms between connect attempts */
#define PHASE1_SLEEP_US     3000000  /* 3 s — let all phase-1 matches form */
#define INTER_STAGE_SLEEP_US 500000  /* 500 ms between queue stages */
#define TEST_TIMEOUT_S      25       /* hard deadline: game lifetime (5 s)
                                        + phase-1 wait (3 s) + stages + margin */

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

static void send_match_request(int sock)
{
    uint8_t buf[TCP_SEGMENT_SIZE];
    memset(buf, 0, sizeof(buf));
    struct IncomingPacket req = { .event_type = SV_EVENT_MATCH_REQUEST };
    memcpy(buf, &req, sizeof(req));
    if (send(sock, buf, sizeof(buf), 0) != (ssize_t)sizeof(buf))
        die("send");
}

static void close_client(int sock)
{
    shutdown(sock, SHUT_WR);
    char drain[64];
    while (recv(sock, drain, sizeof(drain), 0) > 0)
        ;
    close(sock);
}

/* Connect n_clients, append their fds to socks[*cursor], send match requests. */
static void run_stage(const char *ip, const char *label,
                      int n_clients, int *socks, int *cursor)
{
    int base = *cursor;
    printf("%s: connecting %d clients (queue total will be %d)...\n",
           label, n_clients, base + n_clients);

    for (int i = 0; i < n_clients; i++)
    {
        socks[base + i] = connect_client(ip);
        if (socks[base + i] < 0)
        {
            fprintf(stderr, "FAIL: %s client %d could not connect after %d attempts\n",
                    label, i, CONNECT_RETRIES);
            exit(EXIT_FAILURE);
        }
    }

    for (int i = 0; i < n_clients; i++)
        send_match_request(socks[base + i]);

    *cursor += n_clients;
}

int main(void)
{
    signal(SIGALRM, on_timeout);
    alarm(TEST_TIMEOUT_S);

    const char *ip = IP_ADDRESS;
    int phase1_socks[PHASE1_CLIENTS];
    int queued_socks[TOTAL_QUEUED];
    int pass = 1;

    /* ------------------------------------------------------------------ */
    /* Phase 1: exhaust the port pool                                       */
    /* ------------------------------------------------------------------ */
    printf("=== Phase 1: filling all %d ports (%d clients) ===\n",
           PORTS_LIMIT, PHASE1_CLIENTS);

    for (int i = 0; i < PHASE1_CLIENTS; i++)
    {
        phase1_socks[i] = connect_client(ip);
        if (phase1_socks[i] < 0)
        {
            fprintf(stderr, "FAIL: phase-1 client %d could not connect after %d attempts\n",
                    i, CONNECT_RETRIES);
            exit(EXIT_FAILURE);
        }
    }

    for (int i = 0; i < PHASE1_CLIENTS; i++)
        send_match_request(phase1_socks[i]);

    usleep(PHASE1_SLEEP_US);

    for (int i = 0; i < PHASE1_CLIENTS; i++)
    {
        uint8_t rbuf[TCP_SEGMENT_SIZE];
        ssize_t n = recv(phase1_socks[i], rbuf, sizeof(rbuf), 0);
        if (n < (ssize_t)sizeof(struct OutgoingPacket))
        {
            fprintf(stderr, "FAIL: phase-1 client %d received %zd bytes\n", i, n);
            pass = 0;
            continue;
        }
        struct OutgoingPacket pkt;
        memcpy(&pkt, rbuf, sizeof(pkt));
        if (pkt.event_type != SV_EVENT_MATCH_FOUND)
        {
            fprintf(stderr, "FAIL: phase-1 client %d got event_type=%u\n",
                    i, pkt.event_type);
            pass = 0;
        }
    }

    if (!pass)
    {
        fprintf(stderr, "FAIL: phase 1 did not succeed — cannot test queue behaviour\n");
        for (int i = 0; i < PHASE1_CLIENTS; i++) close_client(phase1_socks[i]);
        return 1;
    }

    printf("PASS: phase 1 — all %d ports in use\n\n", PORTS_LIMIT);

    /* ------------------------------------------------------------------ */
    /* Queue stages: arrive in waves while the port pool is empty           */
    /* ------------------------------------------------------------------ */
    printf("=== Queue stages: %d clients total across 4 uneven stages ===\n",
           TOTAL_QUEUED);
    printf("    Odd stage sizes force players from different stages to pair.\n\n");

    int cursor = 0;

    run_stage(ip, "Stage 2 (7, odd)", STAGE2_CLIENTS, queued_socks, &cursor);
    printf("    Queue: %d players, %d complete pairs, %d waiting for partner\n\n",
           cursor, cursor / PLAYERS_PER_MATCH, cursor % PLAYERS_PER_MATCH);
    usleep(INTER_STAGE_SLEEP_US);

    run_stage(ip, "Stage 3 (5, odd)", STAGE3_CLIENTS, queued_socks, &cursor);
    printf("    Queue: %d players, %d complete pairs, %d waiting for partner\n\n",
           cursor, cursor / PLAYERS_PER_MATCH, cursor % PLAYERS_PER_MATCH);
    usleep(INTER_STAGE_SLEEP_US);

    run_stage(ip, "Stage 4 (9, odd)", STAGE4_CLIENTS, queued_socks, &cursor);
    printf("    Queue: %d players, %d complete pairs, %d waiting for partner\n\n",
           cursor, cursor / PLAYERS_PER_MATCH, cursor % PLAYERS_PER_MATCH);
    usleep(INTER_STAGE_SLEEP_US);

    run_stage(ip, "Stage 5 (3, odd)", STAGE5_CLIENTS, queued_socks, &cursor);
    printf("    Queue: %d players, %d complete pairs, %d waiting for partner\n\n",
           cursor, cursor / PLAYERS_PER_MATCH, cursor % PLAYERS_PER_MATCH);

    printf("All %d clients queued (%d matches pending).\n"
           "Blocking on recv — waiting for phase-1 game processes to exit "
           "and return ports...\n\n",
           TOTAL_QUEUED, TOTAL_QUEUED / PLAYERS_PER_MATCH);

    /* ------------------------------------------------------------------ */
    /* Read responses — unblocks once ports are returned and matches form   */
    /* ------------------------------------------------------------------ */
    for (int i = 0; i < TOTAL_QUEUED; i++)
    {
        uint8_t rbuf[TCP_SEGMENT_SIZE];
        ssize_t n = recv(queued_socks[i], rbuf, sizeof(rbuf), 0);

        if (n < (ssize_t)sizeof(struct OutgoingPacket))
        {
            fprintf(stderr, "FAIL: queued client %d received %zd bytes (expected >= %zu)\n",
                    i, n, sizeof(struct OutgoingPacket));
            pass = 0;
            continue;
        }

        struct OutgoingPacket pkt;
        memcpy(&pkt, rbuf, sizeof(pkt));

        if (pkt.event_type != SV_EVENT_MATCH_FOUND)
        {
            fprintf(stderr, "FAIL: queued client %d got event_type=%u (expected MATCH_FOUND)\n",
                    i, pkt.event_type);
            pass = 0;
        }
        else
        {
            printf("  queued client %2d: MATCH FOUND  port=%-5u  game=%02x%02x%02x%02x\n",
                   i, (unsigned)pkt.server_port,
                   pkt.game_id[0], pkt.game_id[1], pkt.game_id[2], pkt.game_id[3]);
        }
    }

    /* Cleanup */
    for (int i = 0; i < PHASE1_CLIENTS; i++) close_client(phase1_socks[i]);
    for (int i = 0; i < TOTAL_QUEUED;   i++) close_client(queued_socks[i]);

    printf("\n");
    if (pass)
        printf("PASS: all %d queued clients matched after ports became available\n",
               TOTAL_QUEUED);
    else
        printf("FAIL: one or more queued clients did not receive MATCH_FOUND\n");

    return pass ? 0 : 1;
}
