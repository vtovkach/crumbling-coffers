#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/random.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "server-config.h"

#define SUPERVISOR_BIN      "./bin/supervisor"
#define CONNECT_RETRIES     20
#define CONNECT_RETRY_US    50000  /* 50 ms between connect attempts (1 s total) */
#define POST_SEND_SLEEP_US  50000  /* 50 ms — let supervisor process the data */
#define NUM_CLIENTS         20

static void die(const char *msg)
{
    perror(msg);
    exit(EXIT_FAILURE);
}

int main(void)
{
    /* Pipe keeps supervisor's shutdown_controller blocked on poll —
     * if stdin is /dev/null, getline hits EOF immediately and the
     * supervisor shuts down before the orchestrator binds. */
    int stdin_pipe[2];
    if (pipe(stdin_pipe) < 0)
        die("pipe");

    pid_t spid = fork();
    if (spid < 0)
        die("fork");

    if (spid == 0)
    {
        close(stdin_pipe[1]);
        dup2(stdin_pipe[0], STDIN_FILENO);
        close(stdin_pipe[0]);

        int devnull = open("/dev/null", O_WRONLY);
        if (devnull >= 0)
        {
            dup2(devnull, STDOUT_FILENO);
            dup2(devnull, STDERR_FILENO);
            close(devnull);
        }
        execl(SUPERVISOR_BIN, SUPERVISOR_BIN, NULL);
        perror("execl supervisor");
        _exit(1);
    }

    /* Parent holds write end open; supervisor blocks waiting for input. */
    close(stdin_pipe[0]);

    /* --- connect with retry --- */
    struct sockaddr_in addr = {0};
    addr.sin_family      = AF_INET;
    addr.sin_port        = htons(atoi(SERVER_TCP_PORT));
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

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
        close(stdin_pipe[1]);
        kill(spid, SIGKILL);
        waitpid(spid, NULL, 0);
        exit(EXIT_FAILURE);
    }

    /* First connection already open — send its data, then open the rest. */
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

    /* --- each client sends TCP_SEGMENT_SIZE bytes then closes --- */
    for (int i = 0; i < NUM_CLIENTS; i++)
    {
        uint8_t buf[TCP_SEGMENT_SIZE];
        ssize_t rng = getrandom(buf, sizeof(buf), 0);
        if (rng != TCP_SEGMENT_SIZE)
            die("getrandom");

        ssize_t sent = send(socks[i], buf, TCP_SEGMENT_SIZE, 0);
        if (sent != TCP_SEGMENT_SIZE)
            die("send");

        /* Graceful close: shutdown write side, drain any server reply, then close. */
        shutdown(socks[i], SHUT_WR);
        char drain[64];
        while (recv(socks[i], drain, sizeof(drain), 0) > 0)
            ;
        close(socks[i]);
    }

    usleep(POST_SEND_SLEEP_US);

    /* --- assert supervisor survived --- */
    int wstatus;
    pid_t ret = waitpid(spid, &wstatus, WNOHANG);
    if (ret != 0)
    {
        fprintf(stderr, "FAIL: supervisor exited after receiving input\n");
        exit(EXIT_FAILURE);
    }

    printf("PASS: supervisor alive after %d clients each sending %d bytes\n",
           NUM_CLIENTS, TCP_SEGMENT_SIZE);

    sleep(5);
    close(stdin_pipe[1]);
    kill(spid, SIGKILL);
    waitpid(spid, NULL, 0);

    return 0;
}
