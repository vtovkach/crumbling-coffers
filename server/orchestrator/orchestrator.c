#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <netinet/tcp.h>
#include <fcntl.h>

#define LOG_FILE "log/orchestrator"

#define SERVER_TCP_PORT "10000"
#define MAX_TCP_QUEUE   128

struct Client
{
    int fd;
    struct sockaddr_in addr;
    
    // Define other necessary fields later 
};

volatile sig_atomic_t terminate = 0;

void set_terminate(int sig)
{
    (void)sig; // silences the compiler warning  
    terminate = 1;
}

int main(int argc, char *argv[])
{
    if(argc != 2)
    {
        printf("orchestrator: incorrect argument list.\n");
        return 1;
    }
    
    // Obtain parent pid 
    pid_t p_pid = (pid_t)atoi(argv[1]);

    // Connect the signal to a handler routine to allow supervisor process gracefully terminate the orchestrator process 
    if(signal(SIGUSR1, set_terminate) == SIG_ERR)
    {
        perror("signal (orchestrator)");
        kill(p_pid, SIGUSR2);
        return 1;
    }

    // Open log file for orchestrator process 
    FILE *log_file = fopen(LOG_FILE, "a");
    if(!log_file)
    {
        // Critical Error
        perror("fopen (orchestrator)"); 
        kill(p_pid, SIGUSR2);
        return 1; 
    }

    // Establish and bind listening socket to designated PORT 
    
    int listen_fd; 
    struct addrinfo *listen_ai = NULL;
    struct addrinfo hint;  

    memset(&hint, 0, sizeof(hint));
    hint.ai_family = AF_INET;
    hint.ai_socktype = SOCK_STREAM;
    hint.ai_protocol = 0; 
    hint.ai_flags = AI_PASSIVE;

    if(getaddrinfo(NULL, SERVER_TCP_PORT, &hint, &listen_ai) != 0)
    {
        perror("getaddrinfo (orchestrator)");
        kill(p_pid, SIGUSR2);
        return 1; 
    }

    listen_fd = socket(listen_ai->ai_family, listen_ai->ai_socktype, listen_ai->ai_protocol);
    if(listen_fd < 0)
        goto error; 

    int opt = 1; 
    if(setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
        goto error; 

    if(bind(listen_fd, listen_ai->ai_addr, listen_ai->ai_addrlen) == -1)
        goto error;

    if(listen(listen_fd, MAX_TCP_QUEUE) == -1)
        goto error; 
    
    int fl = fcntl(listen_fd, F_GETFL, 0);
    if(fl < 0 || fcntl(listen_fd, F_SETFL, fl | O_NONBLOCK))
        goto error; 
    
    for(;;)
    {
        // Check if parent wants to terminate the process 
        if(terminate)
        {
            // Later gracefully terminate this process 

            close(listen_fd);
            freeaddrinfo(listen_ai);

            break;
        }

        sleep(2);
    }

    return 0;

error:
    perror("socket/bind/listen/fcntl failed (orchestrator)");
    kill(p_pid, SIGUSR2);
    freeaddrinfo(listen_ai);
    return 1;
}