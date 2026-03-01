#include <stdio.h>                          
#include "orchestrator/state/client.h"
#include <unistd.h>      
#include <sys/types.h>

#include "server-config.h"            

int addClientToQueue(struct Client *client)
{
    // Just a place holder for now
    // TODO 
    
    printf("Buffer's Content: ");
    for(int i = 0; i < TCP_SEGMENT_SIZE; i++)
    {
        char cur_char = (char) client->buffer[i];
        if(cur_char == '\0') { cur_char = '*'; } // Indicate pending zero 
        
        putchar(cur_char);
    }
    putchar('\n');
    fflush(stdout);

    /*
    // For now, only used for 'testing' 
    // Spawning game process 
    pid_t p_pid = getpid();
    pid_t g_pid = fork();
    if(g_pid < 0)
    {
        // Error happened 
        perror("[addClientQueue] fork");
    }

    if(g_pid == 0)
    {
        char string_p_pid[32];
        char string_pipe_fd[32];

        snprintf(string_p_pid, sizeof(string_p_pid), "%d", p_pid);
        snprintf(string_pipe_fd, sizeof(string_pipe_fd), "%d", 476);

        char *argv[] = {
            GAME_PROCESS,
            string_p_pid,
            UDP_GAME_PROCESS_PORT,
            string_pipe_fd,
            NULL
        };

        execv(GAME_PROCESS, argv);

        // Runs only if execv fails 
        perror("[addClientQueue] execv");
        _exit(1);
    }
    */
   
    return 0;
    
}