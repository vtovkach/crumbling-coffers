#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/random.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "server-config.h"
#include "test-config.h"

#define CONNECT_RETRIES      20
#define CONNECT_RETRY_US     0   /* 50 ms between connect attempts (1 s total) */
#define INTER_SEND_SLEEP_US  0   /* 10 ms between each client send */
#define POST_SEND_SLEEP_US   100000  /* 500 ms after all sends before disconnecting */
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

    /* --- connect with retry (supervisor may still be starting up) --- */
    int sock = -1;
    for (int i = 0; i < CONNECT_RETRIES; i++)
    {
        sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0)
            die("socket");

        if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) == 0)
            break;

        close(sock);
        sock = -1;
        usleep(CONNECT_RETRY_US);
    }

    if (sock < 0)
    {
        fprintf(stderr, "FAIL: could not connect to supervisor after %d attempts\n",
                CONNECT_RETRIES);
        exit(EXIT_FAILURE);
    }

    /* First connection already open — open the rest. */
    int socks[NUM_CLIENTS];
    socks[0] = sock;

    for (int i = 1; i < NUM_CLIENTS; i++)
    {
        socks[i] = socket(AF_INET, SOCK_STREAM, 0);
        if (socks[i] < 0)
            die("socket");
        if (connect(socks[i], (struct sockaddr *)&addr, sizeof(addr)) < 0)
            die("connect");
    }

    /* --- each client sends TCP_SEGMENT_SIZE bytes --- */
    for (int i = 0; i < NUM_CLIENTS; i++)
    {
        uint8_t buf[TCP_SEGMENT_SIZE];
        ssize_t rng = getrandom(buf, sizeof(buf), 0);
        if (rng != TCP_SEGMENT_SIZE)
            die("getrandom");

        ssize_t sent = send(socks[i], buf, TCP_SEGMENT_SIZE, 0);
        if (sent != TCP_SEGMENT_SIZE)
            die("send");

        usleep(INTER_SEND_SLEEP_US);
    }

    /* --- wait 500 ms then disconnect all clients --- */
    usleep(POST_SEND_SLEEP_US);

    for (int i = 0; i < NUM_CLIENTS; i++)
    {
        shutdown(socks[i], SHUT_WR);
        char drain[64];
        while (recv(socks[i], drain, sizeof(drain), 0) > 0)
            ;
        close(socks[i]);
    }

    printf("PASS: supervisor handled %d clients each sending %d bytes\n",
           NUM_CLIENTS, TCP_SEGMENT_SIZE);

    return 0;
}
