#include <stdatomic.h>
#include <stdint.h>
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
#include <sys/eventfd.h>

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

static int register_epoll_fds(  FILE *log_file,
                                int efd,
                                int lfd,
                                int orch_eventfd,
                                int send_eventfd,
                                int recv_eventfd
                            )
{
    if(add_fd_epoll(log_file, efd, lfd, EPOLLIN) < 0)           return -1;
    if(add_fd_epoll(log_file, efd, orch_eventfd, EPOLLIN) < 0)  return -1;
    if(add_fd_epoll(log_file, efd, send_eventfd, EPOLLIN) < 0)  return -1;
    if(add_fd_epoll(log_file, efd, recv_eventfd, EPOLLIN) < 0)  return -1;
    return 0;
}

static void accept_connections( FILE *log_file, 
                                int listen_fd,
                                struct ConnController *cc, 
                                struct BufferController *bc
                            )
{
    int num_fds; 
    int *fds = cc_accept_connection(cc, listen_fd, log_file, &num_fds);

    for(int i = 0; i < num_fds; i++)
    {
        bc_add(bc, fds[i]);
    }

    free(fds);
}

static void process_broker(FILE *log_file)
{
    log_message(log_file, "Process from broker!");
};

static void send_data(FILE *log_file)
{
    log_message(log_file, "Sending Data");
};

static void process_usr_request(FILE *log_file)
{
    log_message(log_file, "Process user request");
}

static void read_incoming_data(FILE *log_file)
{
    log_message(log_file, "read_incoming_data");
}

static int process_events( int n_events,
                    int lfd,
                    int send_eventfd,
                    int recv_eventfd,
                    struct OrchArgs *orch_args,
                    struct BufferController *c_buf,
                    struct ConnController *c_con,
                    struct epoll_event *epoll_events
                )
{
    for(int i = 0; i < n_events; i++)
    {
        int fd            = epoll_events[i].data.fd;
        uint32_t events   = epoll_events[i].events;

        if(fd == lfd)
        {
            if(events & EPOLLIN)
            {
                accept_connections(orch_args->log_file, lfd, c_con, c_buf);
            }
            continue;
        }

        if(fd == orch_args->orch_eventfd)
        {
            if(events & EPOLLIN)
            {
                uint64_t val;
                read(fd, &val, sizeof(val));
                process_broker(orch_args->log_file);
            }
            continue;
        }

        if(fd == send_eventfd)
        {
            if(events & EPOLLIN)
            {
                uint64_t val;
                read(fd, &val, sizeof(val));
                send_data(orch_args->log_file);
            }
            continue;
        }

        if(fd == recv_eventfd)
        {
            if(events & EPOLLIN)
            {
                uint64_t val;
                read(fd, &val, sizeof(val));
                process_usr_request(orch_args->log_file);
            }
            continue;
        }

        // Client fd
        if(events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR))
        {
            // TODO: cc_close_connection — clean up disconnected client
            cc_close_connection(c_con, fd, orch_args->log_file);
            bc_remove(c_buf, fd);
            continue;
        }

        if(events & EPOLLIN)
        {
            read_incoming_data(orch_args->log_file);
        }
    }

    return 0;
}

void *orch_run_t(void *args)
{
    struct OrchArgs *t_orch_args  = (struct OrchArgs *)args;
    FILE           *log           = t_orch_args->log_file;
    int             orch_eventfd  = t_orch_args->orch_eventfd;

    struct BufferController *c_buf = NULL;
    struct ConnController *c_con   = NULL;
    struct epoll_event epoll_events[EPOLL_MAX_EVENTS];

    int lfd          = -1;
    int efd          = -1;
    int send_eventfd = -1;
    int recv_eventfd = -1;

    lfd = setup_listen_socket(log);
    if(lfd < 0) goto exit;

    efd = epoll_create1(0);
    if(efd < 0)
    {
        log_error(log, "[orch_run_t] epoll_create1 failed", errno);
        goto exit;
    }

    c_buf = bc_init();
    if(!c_buf)
    {
        log_error(log, "[orch_run_t] bc_init failed", errno);
        goto exit;
    }

    c_con = cc_init(efd);
    if(!c_con)
    {
        log_error(log, "[orch_run_t] cc_init failed", errno);
        goto exit;
    }

    send_eventfd = eventfd(0, EFD_NONBLOCK);
    if(send_eventfd < 0)
    {
        log_error(log, "[orch_run_t] send_eventfd failed", errno);
        goto exit;
    }

    recv_eventfd = eventfd(0, EFD_NONBLOCK);
    if(recv_eventfd < 0)
    {
        log_error(log, "[orch_run_t] recv_eventfd failed", errno);
        goto exit;
    }

    int ret = register_epoll_fds(
        log, 
        efd, 
        lfd, 
        orch_eventfd, 
        send_eventfd, 
        recv_eventfd
    );
    if(ret != 0) goto exit;

    while(!atomic_load(&t_shutdown))
    {
        int ret = epoll_wait(
            efd, 
            epoll_events, 
            EPOLL_MAX_EVENTS, 
            EPOLL_TIMEOUT
        );

        if(ret == -1)
        {
            log_error(log, "[orch_run_t] epoll_wait critical failure", errno);
            goto exit;
        }

        if(ret == 0) continue;

        int status = process_events(
            ret, 
            lfd, 
            send_eventfd, 
            recv_eventfd, 
            t_orch_args, 
            c_buf, 
            c_con, 
            epoll_events
        );

        if(status < 0)
        {
            log_error(log, "[orch_run_t] process_events: critical failure", 0);
            goto exit;
        }
    }

exit:

    atomic_store(&t_shutdown, true);

    if(c_buf) bc_destroy(c_buf);
    if(c_con) cc_destroy(c_con);
    if(lfd != -1) close(lfd);
    if(efd != -1) close(efd);
    if(send_eventfd != -1) close(send_eventfd);
    if(recv_eventfd != -1) close(recv_eventfd);

    return NULL;
}