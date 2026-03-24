#define _POSIX_C_SOURCE 200809L

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <errno.h>

/*
 * Adjust these to match your real project values.
 */
#define GAME_ID_SIZE   16
#define PLAYER_ID_SIZE 16

static int write_full(int fd, const void *buf, size_t count)
{
    const uint8_t *p = (const uint8_t *)buf;
    size_t total = 0;

    while (total < count)
    {
        ssize_t n = write(fd, p + total, count - total);
        if (n < 0)
        {
            if (errno == EINTR)
                continue;
            return -1;
        }
        if (n == 0)
            return -1;

        total += (size_t)n;
    }

    return 0;
}

static void fill_demo_bytes(uint8_t *buf, size_t size, uint8_t start)
{
    for (size_t i = 0; i < size; i++)
        buf[i] = (uint8_t)(start + i);
}

int main(void)
{
    int pipefds[2];
    pid_t pid;

    uint16_t port = 5000;
    uint8_t game_id[GAME_ID_SIZE];
    uint32_t players_num = 3;
    uint8_t player_ids[PLAYER_ID_SIZE * 3];

    /*
     * Example data:
     * game_id  = 0x10, 0x11, 0x12, ...
     * player 1 = 0x20, 0x21, ...
     * player 2 = 0x30, 0x31, ...
     * player 3 = 0x40, 0x41, ...
     */
    fill_demo_bytes(game_id, GAME_ID_SIZE, 0x10);
    fill_demo_bytes(player_ids + 0 * PLAYER_ID_SIZE, PLAYER_ID_SIZE, 0x20);
    fill_demo_bytes(player_ids + 1 * PLAYER_ID_SIZE, PLAYER_ID_SIZE, 0x30);
    fill_demo_bytes(player_ids + 2 * PLAYER_ID_SIZE, PLAYER_ID_SIZE, 0x40);

    if (pipe(pipefds) < 0)
    {
        perror("pipe");
        return 1;
    }

    pid = fork();
    if (pid < 0)
    {
        perror("fork");
        close(pipefds[0]);
        close(pipefds[1]);
        return 1;
    }

    if (pid == 0)
    {
        char fd_arg[32];

        /*
         * Child:
         * - closes write end
         * - execs the game binary, passing read-end fd as argv[1]
         */
        close(pipefds[1]);

        snprintf(fd_arg, sizeof(fd_arg), "%d", pipefds[0]);

        /*
         * Replace "./bin/game" with the real path to your game executable.
         */
        execl("../../bin/game", "../../bin/game", fd_arg, (char *)NULL);

        perror("execl");
        close(pipefds[0]);
        _exit(1);
    }

    /*
     * Parent:
     * - closes read end
     * - writes data in the exact order expected by game main()
     */
    close(pipefds[0]);

    if (write_full(pipefds[1], &port, sizeof(port)) < 0)
    {
        perror("write port");
        close(pipefds[1]);
        waitpid(pid, NULL, 0);
        return 1;
    }

    if (write_full(pipefds[1], game_id, sizeof(game_id)) < 0)
    {
        perror("write game_id");
        close(pipefds[1]);
        waitpid(pid, NULL, 0);
        return 1;
    }

    if (write_full(pipefds[1], &players_num, sizeof(players_num)) < 0)
    {
        perror("write players_num");
        close(pipefds[1]);
        waitpid(pid, NULL, 0);
        return 1;
    }

    if (write_full(pipefds[1], player_ids, sizeof(player_ids)) < 0)
    {
        perror("write player_ids");
        close(pipefds[1]);
        waitpid(pid, NULL, 0);
        return 1;
    }

    close(pipefds[1]);

    {
        int status;
        if (waitpid(pid, &status, 0) < 0)
        {
            perror("waitpid");
            return 1;
        }

        if (WIFEXITED(status))
        {
            printf("game exited with status %d\n", WEXITSTATUS(status));
        }
        else if (WIFSIGNALED(status))
        {
            printf("game killed by signal %d\n", WTERMSIG(status));
        }
        else
        {
            printf("game ended in unexpected way\n");
        }
    }

    return 0;
}