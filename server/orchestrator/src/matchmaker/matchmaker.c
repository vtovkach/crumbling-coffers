#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "log_system.h"
#include "orchestrator/matchmaker/port_manager.h"
#include "server-config.h"

int spawnGameProcess(struct PortManager *pm, FILE *const log_file, uint16_t port)
{
    // Spawn a child process 
    pid_t c_pid = fork();    
    if(c_pid < 0)
    {
        log_error(log_file, "[spawnGameProcess] ht_insert_port failed.", errno);
        return -1;
    }

    if(c_pid == 0)
    {
        char port_str[64];
        snprintf(port_str, 64, "%d", (int)port);
        char *args[3] = {GAME_PROCESS, port_str, NULL};

        execv(GAME_PROCESS, args);

        // Runs only if execv fails 
        log_error(log_file, "[spawnGameProcess] execv failed.", errno);
        _exit(1);
    }

    // Insert PID to PORT mapping to the hashtable 
    if(ht_insert_port(pm, c_pid, port, log_file) < 0)
    {
        log_message(log_file, "[spawnGameProcess] ht_insert_port failed.");
        return -1;
    }

    char log_msg[128];
    snprintf(log_msg, sizeof(log_msg), "Game process was spawned. Port: %d PID: %d\n", port, (int)c_pid);
    log_message(log_file, log_msg);

    return 0;
}