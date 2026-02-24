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
#include <stdint.h>
#include <stdbool.h>

#include "../include/common/hashmap.h"
#include "../include/common/util.h"

#define LOG_FILE "log/orchestrator"

#define SERVER_TCP_PORT  "10000"
#define MAX_TCP_QUEUE    128
#define MAX_EPOLL_EVENTS 512
#define HASH_TABLE_SIZE  4096

unsigned int hash(const void *key, unsigned int table_size)
{
    uint32_t x;
    memcpy(&x, key, sizeof(uint32_t));

    // Murmur3 finalizer mix
    x ^= x >> 16;
    x *= 0x85ebca6b;
    x ^= x >> 13;
    x *= 0xc2b2ae35;
    x ^= x >> 16;

    return x & (table_size - 1);  // table_size must be power of 2
}

struct Client
{
    int fd;
    struct sockaddr_in addr;

    // Input Data 
    void *input_buffer;
    size_t i_buf_size;   // 150 bytes 
    size_t received_bytes; 
    bool is_fully_received; 

    // Output Data 
    void *output_buffer; // 150 bytes  
    size_t o_buf_size;
    size_t sent_bytes; 
    bool is_fully_sent;  
};

volatile sig_atomic_t terminate = 0;

void set_terminate(int sig)
{
    (void)sig; // silences the compiler warning  
    terminate = 1;
}

int setupListenSocket(void)
{
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
        printf("[setupListenSocket] getaddrinfo failed (orchestrator)");
        freeaddrinfo(listen_ai);
        return -1; 
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

    freeaddrinfo(listen_ai);
    return listen_fd;

error:
    perror("socket/bind/listen/fcntl failed (orchestrator)");
    freeaddrinfo(listen_ai);
    return -1;
}

int acceptConnections(FILE *const log_file, int listen_fd, const int epoll_fd, struct HashTable *const active_clients)
{
    int accepted = 0;

    // Accept all pending connections 
    while(true)
    {
        struct Client new_client;
        struct sockaddr_in c_addr;
        socklen_t ca_len = sizeof(c_addr);
        int conn_fd = accept4(listen_fd, (struct sockaddr *)&c_addr, &ca_len, SOCK_NONBLOCK | SOCK_CLOEXEC);

        if(conn_fd < 0)
        {
            if(errno == EAGAIN || errno == EWOULDBLOCK)
                // no more pending connections in line 
                break; 

            log_error(log_file, "[accept_connections] accept error", errno);

            continue;
        }

        new_client.fd = conn_fd;
        new_client.addr = c_addr;

        // Add connection to the hash map 
        if(ht__insert_internal(active_clients, &conn_fd, &new_client) < 0)
        {
            // hash table failed adding a new key value pair 

            log_error(log_file, "[accept_connections] ht__insert_internal failed", 0);

            close(conn_fd);
            continue;
        }

        // Add connection fd to epoll 
        struct epoll_event ev = {.data.fd = conn_fd, .events = EPOLLIN | EPOLLRDHUP | EPOLLHUP | EPOLLERR}; 
        if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, conn_fd, &ev) == -1)
        {
            int saved_errno = errno; 

            if(ht__remove_internal(active_clients, &conn_fd) != 1)
            {
                close(conn_fd);
                log_error(log_file, "[accept_connections] CRITICAL — unable to remove client from active_clients (state corruption possible)\n", saved_errno);
                accepted = -1;
                break;
            }
            
            close(conn_fd);

            log_error_fd(log_file, "[accept_connections] epoll_ctl error", conn_fd, saved_errno);

            continue;
        }

        // Log connected Client 
        char time[TIME_BUFFER_SIZE];
        getTime(time, TIME_BUFFER_SIZE);

        // Retrieve IP Address and Source Port 
        char ip_str[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &c_addr.sin_addr, ip_str, sizeof(ip_str));
        uint16_t c_port = ntohs(c_addr.sin_port);

        fprintf(log_file, "%s [accept_connections] CONNECT %s:%u fd=%d\n", time, ip_str, c_port, conn_fd);
        accepted++;
    }

    // Flush log_file's buffer 
    fflush(log_file);

    return accepted;
}

int closeConnection(FILE *const log_file, int epoll_fd, int target_fd, struct HashTable *const active_clients)
{    
    if(epoll_ctl(epoll_fd, EPOLL_CTL_DEL, target_fd, NULL) == -1)
    {
        // epoll_ctl failed  
        ht__remove_internal(active_clients, &target_fd);
        close(target_fd);

        log_error_fd(log_file, "epoll_ctl DEL failed", target_fd, errno);

        return -1; // Indicate critical error  
    }

    if(ht__remove_internal(active_clients, &target_fd) <= 0)
    {
        // Hash Table error or element not found 

        close(target_fd);

        log_error_fd(log_file, "Hash table removal failed or element not found (possible memory leak)", target_fd, 0);

        return 1; // Indicate potential memory leak 
    }

    close(target_fd);

    // Log Success 
    log_error_fd(log_file, "Connection closed by peer", target_fd, 0);

    // Flush log_file's buffer  
    fflush(log_file);

    return 0;
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

    struct HashTable *active_clients = ht_create(sizeof(int), 1, sizeof(struct Client), 1, hash, HASH_TABLE_SIZE); 
    if(!active_clients)
    {
        printf("HashTable init failed.\n");
        kill(p_pid, SIGUSR2);
        return 1;
    }

    // Establish and bind listening socket to designated PORT 
    int listen_fd; 
    listen_fd = setupListenSocket();
    if(listen_fd < 0)
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

            break;
        }

        int events_ready = epoll_wait(epoll_fd, eventQueue, MAX_EPOLL_EVENTS, 2000);

        if(events_ready < 0)
            perror("epoll wait error");

        if(events_ready > 0)
        {
            printf("We got some events!\n");

            for(int i = 0; i < events_ready; i++)
            {

                // Check each epoll_event 
                // TODO 

                struct epoll_event cur_event = eventQueue[i];

                if(cur_event.data.fd == listen_fd)
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
                    // Peer disconnected 
                    if(closeConnection(log_file, epoll_fd, cur_event.data.fd, active_clients) < 0)
                    {
                        // Critical Error happened shutdown server 
                        // TODO 
                        // ...
                        printf("shutdown: Critical Error in [closeConnection]\n");
                        break; 
                    }
                }
            }
        }
    }

    return 0;

error:
    printf("[Orchestrator] critical error occurred while setting up orchestrator process.\n");
    kill(p_pid, SIGUSR2);
    return 1; 
}