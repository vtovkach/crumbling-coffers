#define _GNU_SOURCE

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

#include "../include/common/hashmap.h"
#include "../include/common/util.h"

#define LOG_FILE "log/orchestrator"

#define SERVER_TCP_PORT "10000"
#define MAX_TCP_QUEUE   128

#define MAX_EPOLL_EVENTS 512

// FUNCTION PROTOTYPES 
int acceptConnections(FILE *const log_file, const int listen_fd, const int epoll_fd, struct HashTable *const active_clients);


unsigned int hash(const void *, unsigned int)
{
    // TODO 
    return 0;
}

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

    struct HashTable *active_clients = ht_create(sizeof(int), 1, sizeof(struct Client), 1, hash, 512); 
    if(!active_clients)
        goto error;


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
        printf("getaddrinfo failed (orchestrator)");
        kill(p_pid, SIGUSR2);
        return 1; 
    }

    listen_fd = socket(listen_ai->ai_family, listen_ai->ai_socktype, listen_ai->ai_protocol);
    if(listen_fd < 0)
        goto error; 

    int opt = 1; 
    // Tell kernel to make the port immediately reusable after listening socket is closed 
    if(setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
        goto error; 

    if(bind(listen_fd, listen_ai->ai_addr, listen_ai->ai_addrlen) == -1)
        goto error;

    if(listen(listen_fd, MAX_TCP_QUEUE) == -1)
        goto error; 
    
    int fl = fcntl(listen_fd, F_GETFL, 0);
    if(fl < 0 || fcntl(listen_fd, F_SETFL, fl | O_NONBLOCK) < 0)
        goto error; 

    // Set up epoll to monitor and react to events

    int epoll_fd;

    // Event Queue will contain epoll_events that have certain actions on them  
    struct epoll_event eventQueue[MAX_EPOLL_EVENTS];

    epoll_fd = epoll_create1(0);
    if(epoll_fd < 0)
        goto error;

    // Create event_epoll instance for a listening socket 
    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = listen_fd;

    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listen_fd, &ev);

    for(;;)
    {
        // Check if parent wants to terminate the process 
        if(terminate)
        {
            // Later gracefully terminate this process 

            close(epoll_fd);
            close(listen_fd);
            freeaddrinfo(listen_ai);

            break;
        }

        int events_ready = epoll_wait(epoll_fd, eventQueue, MAX_EPOLL_EVENTS, 2000);

        if(events_ready < 0)
            perror("epoll wait error");

        if(events_ready > 0)
        {
            printf("We got some events!");

            for(int i = 0; i < events_ready; i++)
            {

                // Check each epoll_event 
                // TODO 

                struct epoll_event cur_event = eventQueue[i];

                if(cur_event.data.fd & listen_fd)
                {
                    int accepted = acceptConnections(log_file, listen_fd, epoll_fd, active_clients);

                    if(accepted == -1)
                    {
                        // Critical Server Error 
                        // Do Graceful Shutdown 
                        // TODO 
                        // ... 
                        printf("shutdown: Critical Error in [acceptConnections]\n");
                        break; 
                    }

                    printf("Accepted: %d\n", accepted);

                    continue;
                }

                if(cur_event.events & EPOLLIN)
                {
                    // Read received data 
                    // TODO 
                }

                if(cur_event.events & EPOLLOUT)
                {
                    // Socket is writable 
                    // TODO 
                }

                if(cur_event.events & (EPOLLERR | EPOLLHUP | EPOLLRDHUP))
                {
                    // Disconnected 
                    // TODO 
                }
            }
        }
    }

    return 0;

error:
    perror("socket/bind/listen/fcntl/epoll_create failed (orchestrator)");
    kill(p_pid, SIGUSR2);
    freeaddrinfo(listen_ai);
    return 1;
}