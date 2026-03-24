#include <stdio.h>
#include <stdint.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/types.h>
#include <pthread.h>
#include <errno.h>
#include <stdatomic.h>

#include "game.h"
#include "net/net_thread.h"
#include "signals.h"

/**
 * @brief Entry point for the child game process.
 *
 * @param argc Argument count.
 * @param argv Argument vector.
 *
 * Expected arguments:
 *   argv[1] - UDP game port (uint16_t)
 *
 * @return 0 on success, non-zero on error.
 */
int main(int argc, char *argv[])
{
    (void) argc; // Silence compiler warning 
    signals_install(SIGUSR1); 

    uint16_t game_port = (uint16_t) atoi(argv[1]);

    // Define all structures here 
    // Spawn 2 threads here 
    // and wait for both of them to join at the end 

    int status = runGame(game_port);

    return status;
}

// Use waitpid(... WNOHANG) from orchestrator to track when game processes terminated 
// Use signal to notify child to terminate process 