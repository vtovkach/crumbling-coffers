#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <stdbool.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "server-config.h"
#include "packet.h"
#include "test-config.h"

#define GAME_BINARY          "./bin/game"
#define TEST_TIMEOUT_S       30
#define STARTUP_DELAY_US     200000   /* 200ms for game process to bind socket */
#define INTER_SEND_DELAY_US  30000    /* 30ms between per-player sends */
#define TICK_US              16000    /* 16ms — matches server tick rate */
#define INIT_WAIT_MS         3000     /* max wait for first InitPacket */
#define AUTH_WAIT_MS         9000     /* max wait for first AuthPacket (covers 5s lobby) */
#define AUTH_SETTLE_US       80000    /* 80ms (~5 ticks) after sending for server to process */
#define AUTH_CHECKS          5        /* consecutive AuthPackets to validate */

static const uint8_t TEST_GAME_ID[GAME_ID_SIZE] = {
    0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,
    0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,0x10
};

static uint8_t TEST_PLAYER_IDS[PLAYERS_PER_MATCH][PLAYER_ID_SIZE];

static const float    BASE_POS_X[PLAYERS_PER_MATCH] = { 100.0f,  300.0f };
static const float    BASE_POS_Y[PLAYERS_PER_MATCH] = { 200.0f,  400.0f };
static const float    BASE_VEL_X[PLAYERS_PER_MATCH] = {   1.5f,   -3.0f };
static const float    BASE_VEL_Y[PLAYERS_PER_MATCH] = {  -2.0f,    4.5f };
static const uint32_t BASE_SCORE[PLAYERS_PER_MATCH] = {    10,      20   };

static pid_t              game_pid = -1;
static int                player_socks[PLAYERS_PER_MATCH];
static struct sockaddr_in server_addr;

/* Tracks last data sent by each player — used during validation */
static float    last_pos_x[PLAYERS_PER_MATCH];
static float    last_pos_y[PLAYERS_PER_MATCH];
static float    last_vel_x[PLAYERS_PER_MATCH];
static float    last_vel_y[PLAYERS_PER_MATCH];
static uint32_t last_score[PLAYERS_PER_MATCH];

static void on_timeout(int sig)
{
    (void)sig;
    fprintf(stderr, "FAIL: test timed out after %d seconds\n", TEST_TIMEOUT_S);
    if (game_pid > 0) kill(game_pid, SIGTERM);
    exit(EXIT_FAILURE);
}

static void fail(const char *msg)
{
    fprintf(stderr, "FAIL: %s\n", msg);
    if (game_pid > 0) kill(game_pid, SIGTERM);
    exit(EXIT_FAILURE);
}

static void init_player_ids(void)
{
    for (int i = 0; i < PLAYERS_PER_MATCH; i++)
    {
        memset(TEST_PLAYER_IDS[i], 0, PLAYER_ID_SIZE);
        TEST_PLAYER_IDS[i][0]  = (uint8_t)(0xA0 + i);
        TEST_PLAYER_IDS[i][15] = (uint8_t)(i + 1);
    }
}

static pid_t spawn_game(uint16_t port)
{
    int pipefd[2];
    if (pipe(pipefd) < 0) { perror("pipe"); exit(EXIT_FAILURE); }

    pid_t pid = fork();
    if (pid < 0) { perror("fork"); exit(EXIT_FAILURE); }

    if (pid == 0)
    {
        close(pipefd[1]);
        char fd_str[16];
        snprintf(fd_str, sizeof(fd_str), "%d", pipefd[0]);
        execl(GAME_BINARY, GAME_BINARY, fd_str, NULL);
        perror("execl " GAME_BINARY);
        _exit(EXIT_FAILURE);
    }

    close(pipefd[0]);

    uint8_t  flat_ids[PLAYERS_PER_MATCH * PLAYER_ID_SIZE];
    uint32_t players_num = PLAYERS_PER_MATCH;

    for (int i = 0; i < PLAYERS_PER_MATCH; i++)
        memcpy(flat_ids + i * PLAYER_ID_SIZE, TEST_PLAYER_IDS[i], PLAYER_ID_SIZE);

    write(pipefd[1], &port,        sizeof(port));
    write(pipefd[1], TEST_GAME_ID, GAME_ID_SIZE);
    write(pipefd[1], &players_num, sizeof(players_num));
    write(pipefd[1], flat_ids,     sizeof(flat_ids));
    close(pipefd[1]);

    return pid;
}

static void setup_sockets(uint16_t port)
{
    server_addr.sin_family = AF_INET;
    server_addr.sin_port   = htons(port);
    inet_pton(AF_INET, TEST_GAME_IP, &server_addr.sin_addr);

    for (int i = 0; i < PLAYERS_PER_MATCH; i++)
    {
        player_socks[i] = socket(AF_INET, SOCK_DGRAM, 0);
        if (player_socks[i] < 0) { perror("socket"); exit(EXIT_FAILURE); }

        struct sockaddr_in local = {0};
        local.sin_family      = AF_INET;
        local.sin_addr.s_addr = INADDR_ANY;
        local.sin_port        = 0;
        if (bind(player_socks[i], (struct sockaddr *)&local, sizeof(local)) < 0)
        {
            perror("bind");
            exit(EXIT_FAILURE);
        }
    }
}

static void build_init_packet(uint8_t *buf, int player_idx, uint32_t seq)
{
    memset(buf, 0, UDP_DATAGRAM_SIZE);
    struct Header *h = (struct Header *)buf;
    memcpy(h->game_id,   TEST_GAME_ID,                GAME_ID_SIZE);
    memcpy(h->player_id, TEST_PLAYER_IDS[player_idx], PLAYER_ID_SIZE);
    h->control      = CTRL_FLAG_INIT | CTRL_FLAG_RELIABLE;
    h->payload_size = UDP_DATAGRAM_PAYLOAD_SIZE;
    h->seq_num      = seq;
}

static void build_regular_packet(uint8_t *buf, int player_idx, uint32_t seq, int round)
{
    memset(buf, 0, UDP_DATAGRAM_SIZE);
    struct ClientRegularPacket *pkt = (struct ClientRegularPacket *)buf;
    memcpy(pkt->header.game_id,   TEST_GAME_ID,                GAME_ID_SIZE);
    memcpy(pkt->header.player_id, TEST_PLAYER_IDS[player_idx], PLAYER_ID_SIZE);
    pkt->header.control      = 0x0000;
    pkt->header.payload_size = UDP_DATAGRAM_PAYLOAD_SIZE;
    pkt->header.seq_num      = seq;

    pkt->pos_x = BASE_POS_X[player_idx] + (float)round * 5.0f;
    pkt->pos_y = BASE_POS_Y[player_idx] + (float)round * 3.0f;
    pkt->vel_x = BASE_VEL_X[player_idx] + (float)round * 0.1f;
    pkt->vel_y = BASE_VEL_Y[player_idx] - (float)round * 0.1f;
    pkt->score = BASE_SCORE[player_idx]  + (uint32_t)round;

    last_pos_x[player_idx] = pkt->pos_x;
    last_pos_y[player_idx] = pkt->pos_y;
    last_vel_x[player_idx] = pkt->vel_x;
    last_vel_y[player_idx] = pkt->vel_y;
    last_score[player_idx] = pkt->score;
}

static void send_init_packets(uint32_t seq)
{
    for (int i = 0; i < PLAYERS_PER_MATCH; i++)
    {
        if (i > 0) usleep(INTER_SEND_DELAY_US);

        uint8_t buf[UDP_DATAGRAM_SIZE];
        build_init_packet(buf, i, seq);

        ssize_t sent = sendto(player_socks[i], buf, UDP_DATAGRAM_SIZE, 0,
                              (struct sockaddr *)&server_addr, sizeof(server_addr));
        if (sent != UDP_DATAGRAM_SIZE)
            fail("sendto init packet failed");
    }
}

static void send_regular_packets(uint32_t *seq_nums, int round)
{
    for (int i = 0; i < PLAYERS_PER_MATCH; i++)
    {
        if (i > 0) usleep(INTER_SEND_DELAY_US);

        uint8_t buf[UDP_DATAGRAM_SIZE];
        build_regular_packet(buf, i, seq_nums[i], round);

        ssize_t sent = sendto(player_socks[i], buf, UDP_DATAGRAM_SIZE, 0,
                              (struct sockaddr *)&server_addr, sizeof(server_addr));
        if (sent != UDP_DATAGRAM_SIZE)
            fail("sendto regular packet failed");

        seq_nums[i]++;
    }
}

/*
 * Drains all pending packets from all sockets.
 * If any packet matching ctrl_flag is found, the last one is written to buf_out.
 * Returns 0 if at least one matching packet was found, -1 otherwise.
 */
static int drain_for_latest(uint16_t ctrl_flag, uint8_t *buf_out)
{
    int found = 0;

    for (int i = 0; i < PLAYERS_PER_MATCH; i++)
    {
        uint8_t tmp[UDP_DATAGRAM_SIZE];
        ssize_t n;

        while ((n = recv(player_socks[i], tmp, UDP_DATAGRAM_SIZE, MSG_DONTWAIT)) == UDP_DATAGRAM_SIZE)
        {
            struct Header *h = (struct Header *)tmp;
            if (h->control & ctrl_flag)
            {
                memcpy(buf_out, tmp, UDP_DATAGRAM_SIZE);
                found = 1;
            }
        }
    }

    return found ? 0 : -1;
}

/*
 * Polls all sockets for a packet with ctrl_flag set.
 * Returns 0 and writes to buf_out on success, -1 on timeout.
 */
static int poll_for_ctrl(uint16_t ctrl_flag, uint8_t *buf_out, int timeout_ms)
{
    int elapsed = 0;

    while (elapsed < timeout_ms)
    {
        if (drain_for_latest(ctrl_flag, buf_out) == 0)
            return 0;

        usleep(5000);
        elapsed += 5;
    }

    return -1;
}

static void validate_init_packet(const uint8_t *buf)
{
    const struct InitPacket *pkt = (const struct InitPacket *)buf;

    if (!(pkt->header.control & CTRL_FLAG_INIT))
        fail("InitPacket: CTRL_FLAG_INIT not set");

    if (memcmp(pkt->header.game_id, TEST_GAME_ID, GAME_ID_SIZE) != 0)
        fail("InitPacket: game_id mismatch");

    if (pkt->n != PLAYERS_PER_MATCH)
        fail("InitPacket: wrong player count");

    if (pkt->start_tick == 0 || pkt->stop_tick <= pkt->start_tick)
        fail("InitPacket: invalid tick range");

    for (int i = 0; i < (int)pkt->n; i++)
    {
        bool found = false;
        for (int j = 0; j < PLAYERS_PER_MATCH; j++)
        {
            if (memcmp(pkt->players[i].player_id, TEST_PLAYER_IDS[j], PLAYER_ID_SIZE) == 0)
            {
                found = true;
                break;
            }
        }
        if (!found)
            fail("InitPacket: unrecognised player_id in record");
    }

    printf("  InitPacket OK: n=%u  start_tick=%u  stop_tick=%u\n",
           pkt->n, pkt->start_tick, pkt->stop_tick);
}

static void validate_auth_packet(const uint8_t *buf, int check)
{
    const struct AuthPacket *pkt = (const struct AuthPacket *)buf;

    if (!(pkt->header.control & CTRL_FLAG_AUTH))
        fail("AuthPacket: CTRL_FLAG_AUTH not set");

    if (memcmp(pkt->header.game_id, TEST_GAME_ID, GAME_ID_SIZE) != 0)
        fail("AuthPacket: game_id mismatch");

    if (pkt->n != PLAYERS_PER_MATCH)
        fail("AuthPacket: wrong player count");

    for (int i = 0; i < (int)pkt->n; i++)
    {
        const struct AuthPlayerRecord *rec = &pkt->players[i];

        int pidx = -1;
        for (int j = 0; j < PLAYERS_PER_MATCH; j++)
        {
            if (memcmp(rec->player_id, TEST_PLAYER_IDS[j], PLAYER_ID_SIZE) == 0)
            {
                pidx = j;
                break;
            }
        }

        if (pidx < 0)
            fail("AuthPacket: unrecognised player_id in record");

        if (memcmp(&rec->pos_x, &last_pos_x[pidx], sizeof(float)) != 0 ||
            memcmp(&rec->pos_y, &last_pos_y[pidx], sizeof(float)) != 0 ||
            memcmp(&rec->vel_x, &last_vel_x[pidx], sizeof(float)) != 0 ||
            memcmp(&rec->vel_y, &last_vel_y[pidx], sizeof(float)) != 0 ||
            rec->score != last_score[pidx])
        {
            fprintf(stderr,
                "FAIL: AuthPacket check %d — player %d data mismatch\n"
                "  sent: pos=(%.2f,%.2f) vel=(%.2f,%.2f) score=%u\n"
                "  got:  pos=(%.2f,%.2f) vel=(%.2f,%.2f) score=%u\n",
                check, pidx,
                last_pos_x[pidx], last_pos_y[pidx],
                last_vel_x[pidx], last_vel_y[pidx], last_score[pidx],
                rec->pos_x, rec->pos_y,
                rec->vel_x, rec->vel_y, rec->score);
            fail("AuthPacket data mismatch");
        }
    }

    printf("  AuthPacket OK (check %d): seq=%u\n", check, pkt->header.seq_num);
}

int main(void)
{
    signal(SIGALRM, on_timeout);
    alarm(TEST_TIMEOUT_S);

    init_player_ids();
    setup_sockets(TEST_GAME_PORT);

    game_pid = spawn_game(TEST_GAME_PORT);
    usleep(STARTUP_DELAY_US);

    /* --- Phase 1: connect all players --- */
    printf("Phase 1: connecting players...\n");

    send_init_packets(1);

    uint8_t pkt_buf[UDP_DATAGRAM_SIZE];
    if (poll_for_ctrl(CTRL_FLAG_INIT, pkt_buf, INIT_WAIT_MS) != 0)
        fail("no InitPacket received — players may not have connected");

    validate_init_packet(pkt_buf);

    /* --- Phase 2: send regular packets until game reaches STARTED --- */
    printf("Phase 2: waiting for game to start...\n");

    uint32_t seq_nums[PLAYERS_PER_MATCH];
    for (int i = 0; i < PLAYERS_PER_MATCH; i++)
        seq_nums[i] = 2;  /* INIT used seq=1 */

    int round        = 0;
    int wait_elapsed = 0;
    bool got_auth    = false;

    while (wait_elapsed < AUTH_WAIT_MS && !got_auth)
    {
        round++;
        send_regular_packets(seq_nums, round);

        if (drain_for_latest(CTRL_FLAG_AUTH, pkt_buf) == 0)
        {
            got_auth = true;
            break;
        }

        usleep(TICK_US);
        wait_elapsed += TICK_US / 1000 + (INTER_SEND_DELAY_US / 1000) * (PLAYERS_PER_MATCH - 1);
    }

    if (!got_auth)
        fail("no AuthPacket received — game may not have reached STARTED phase");

    /* --- Phase 3: validate AUTH_CHECKS consecutive AuthPackets --- */
    printf("Phase 3: validating %d AuthPackets with distinct player data...\n", AUTH_CHECKS);

    for (int check = 0; check < AUTH_CHECKS; check++)
    {
        round++;

        /*
         * Send updated data from every player, then wait long enough for the
         * server to process all packets and start broadcasting fresh AuthPackets.
         * Player 1's packet is sent INTER_SEND_DELAY_US after player 0, so the
         * settle must cover that delay plus at least one server tick (16 ms).
         */
        send_regular_packets(seq_nums, round);
        usleep(AUTH_SETTLE_US);

        /*
         * Drain all packets buffered during the settle period — they may include
         * partially-updated state (e.g. player 0 at round N but player 1 still
         * at N-1 if the tick fired between the two sends).  The next packet we
         * receive after the drain is guaranteed to post-date both sends.
         */
        {
            uint8_t scratch[UDP_DATAGRAM_SIZE];
            drain_for_latest(CTRL_FLAG_AUTH, scratch);
        }

        if (poll_for_ctrl(CTRL_FLAG_AUTH, pkt_buf, 500) != 0)
            fail("no AuthPacket received during validation phase");

        validate_auth_packet(pkt_buf, check);
    }

    /* --- Cleanup --- */
    kill(game_pid, SIGTERM);
    waitpid(game_pid, NULL, 0);

    for (int i = 0; i < PLAYERS_PER_MATCH; i++)
        close(player_socks[i]);

    printf("PASS\n");
    return 0;
}
