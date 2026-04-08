#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/random.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>

#include "server-config.h"
#include "test-config.h"

#define CONNECT_RETRIES      20
#define CONNECT_RETRY_US     50000  /* 50 ms between connect attempts */
#define CHUNK_SIZE           25    /* bytes per send call */
#define INTER_CHUNK_SLEEP_US 0
#define POST_SEND_SLEEP_US   0
#define NUM_CLIENTS          10

static void die(const char *msg)
{
    perror(msg);
    exit(EXIT_FAILURE);
}

int main(void)
{
    struct sockaddr_in addr = {0};
    addr.sin_family      = AF_INET;
    addr.sin_port        = htons(atoi(SERVER_TCP_PORT));
    addr.sin_addr.s_addr = inet_addr(IP_ADDRESS);

    int socks[NUM_CLIENTS];

    /* First connection: retry until supervisor is up */
    socks[0] = -1;
    for (int r = 0; r < CONNECT_RETRIES; r++)
    {
        socks[0] = socket(AF_INET, SOCK_STREAM, 0);
        if (socks[0] < 0)
            die("socket");
        if (connect(socks[0], (struct sockaddr *)&addr, sizeof(addr)) == 0)
            break;
        close(socks[0]);
        socks[0] = -1;
        usleep(CONNECT_RETRY_US);
    }

    if (socks[0] < 0)
    {
        fprintf(stderr, "FAIL: could not connect to supervisor after %d attempts\n",
                CONNECT_RETRIES);
        exit(EXIT_FAILURE);
    }

    for (int i = 1; i < NUM_CLIENTS; i++)
    {
        socks[i] = socket(AF_INET, SOCK_STREAM, 0);
        if (socks[i] < 0)
            die("socket");
        if (connect(socks[i], (struct sockaddr *)&addr, sizeof(addr)) < 0)
            die("connect");
    }

    /* Disable Nagle on all sockets so small sends are not coalesced */
    for (int i = 0; i < NUM_CLIENTS; i++)
    {
        int one = 1;
        if (setsockopt(socks[i], IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one)) < 0)
            die("setsockopt TCP_NODELAY");
    }

    /* Generate one random payload shared across all clients */
    uint8_t payload[TCP_SEGMENT_SIZE];
    if (getrandom(payload, sizeof(payload), 0) != TCP_SEGMENT_SIZE)
        die("getrandom");

    /* Each client sends TCP_SEGMENT_SIZE bytes in CHUNK_SIZE pieces with a
     * short sleep between each send, forcing the server to accumulate data
     * across multiple recv() calls rather than reading it all at once. */
    int total_chunks = TCP_SEGMENT_SIZE / CHUNK_SIZE;

    for (int i = 0; i < NUM_CLIENTS; i++)
    {
        for (int c = 0; c < total_chunks; c++)
        {
            ssize_t sent = send(socks[i], payload + c * CHUNK_SIZE, CHUNK_SIZE, 0);
            if (sent != CHUNK_SIZE)
                die("send");
            usleep(INTER_CHUNK_SLEEP_US);
        }
    }

    usleep(POST_SEND_SLEEP_US);

    for (int i = 0; i < NUM_CLIENTS; i++)
    {
        shutdown(socks[i], SHUT_WR);
        char drain[64];
        while (recv(socks[i], drain, sizeof(drain), 0) > 0)
            ;
        close(socks[i]);
    }

    printf("PASS: supervisor handled %d clients each sending %d bytes in %d-byte chunks\n",
           NUM_CLIENTS, TCP_SEGMENT_SIZE, CHUNK_SIZE);

    return 0;
}
