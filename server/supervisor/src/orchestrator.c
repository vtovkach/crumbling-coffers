#include <stdatomic.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/epoll.h>

#include "log_system.h"
#include "server-config.h"
#include "orchestrator.h"
#include "buffer_controller.h"
#include "conn_controller.h"

extern atomic_bool t_shutdown;

static int setup_listen_socket(FILE *log_file)
{
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(listen_fd < 0)
    {
        log_error(log_file, "[setupListenSocket] socket failed", errno);
        return -1;
    }

    int opt = 1;
    // Tell kernel to make the port immediately reusable
    // after listening socket is closed
    if(setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
        goto error;

    struct sockaddr_in addr = {0};
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons(atoi(SERVER_TCP_PORT));

    if(bind(listen_fd, (struct sockaddr *)&addr, sizeof(addr)) == -1)
        goto error;

    if(listen(listen_fd, MAX_TCP_QUEUE) == -1)
        goto error;

    int fl = fcntl(listen_fd, F_GETFL, 0);
    if(fl < 0 || fcntl(listen_fd, F_SETFL, fl | O_NONBLOCK) < 0)
        goto error;

    return listen_fd;

error:
    log_error(log_file, "[setupListenSocket] failed", errno);
    close(listen_fd);
    return -1;
}

static inline int add_fd_epoll(FILE *log_file, int efd, int tfd, int events)
{
    struct epoll_event ev = {
        .data.fd = tfd,
        .events = events,
    };

    int ret = epoll_ctl(efd, EPOLL_CTL_ADD, tfd, &ev);
    if(ret != 0)
    {
        log_error(log_file, "[add_fd_epoll] epoll_ctl failed", errno);
        return -1;
    }

    return 0;
}

void process_events(void)
{
    // process events here 
}

void *orch_run_t(void *args)
{
    struct OrchArgs *t_orch_args = (struct OrchArgs *)args;

    struct BufferController *c_buf = NULL;
    struct ConnController *c_con = NULL;
    struct epoll_event epoll_events[EPOLL_MAX_EVENTS];

    int lfd = -1;
    int efd = -1;

    lfd = setup_listen_socket(t_orch_args->log_file);
    if(lfd < 0) goto exit;

    efd = epoll_create1(0);
    if(efd < 0) goto exit; 

    c_buf = bc_init();
    if(!c_buf) goto exit;

    c_con = cc_init(efd);
    if(!c_con) goto exit; 

    // Monitor listening file descriptor with epoll 
    int ret = add_fd_epoll(t_orch_args->log_file, efd, lfd, EPOLLIN);
    if(ret != 0) goto exit;

    while(!atomic_load(&t_shutdown))
    {

    }

exit:

    atomic_store(&t_shutdown, true);

    if(c_buf) bc_destroy(c_buf);
    if(c_con) cc_destroy(c_con);
    if(lfd != -1) close(lfd);
    if(efd != -1) close(efd);

    return NULL;
}