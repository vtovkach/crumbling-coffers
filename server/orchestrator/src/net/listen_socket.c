#include <stdio.h>      
#include <string.h>    
#include <unistd.h>     
#include <fcntl.h>      
#include <sys/types.h>  
#include <sys/socket.h> 
#include <netdb.h>      
#include <netinet/in.h> 
#include <errno.h>

#include "server-config.h" 

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
    perror("[setupListenSocket] socket/bind/listen/fcntl failed");
    freeaddrinfo(listen_ai);
    return -1;
}
