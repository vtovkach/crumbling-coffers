#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>

#include "threads/game.h"


/**
 * @brief Entry point for the child game process.
 *
 * @param argc Argument count.
 * @param argv Argument vector.
 *
 * Expected arguments:
 *   argv[1] - Parent process PID (pid_t)
 *   argv[2] - UDP game port (uint16_t)
 *   argv[3] - pipefd[1]
 *
 * @return 0 on success, non-zero on error.
 */
int main(int argc, char *argv[])
{
    pid_t p_pid = (pid_t) atoi(argv[1]); 
    uint16_t game_port = (uint16_t) atoi(argv[2]);
    int pipe_fd = atoi(argv[3]);
    
    int status = runGame(game_port);

    return status;
}