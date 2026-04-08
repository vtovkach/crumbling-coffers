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
#include <arpa/inet.h>

#include "log_system.h"
#include "server-config.h"
#include "orchestrator/orchestrator.h"
#include "orchestrator/buffer_controller.h"
#include "orchestrator/conn_controller.h"
#include "orchestrator/io.h"
#include "broker.h"
#include "broker-config.h"
#include "tcp_packets.h"
#include "orchestrator/fd_queue.h"

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

static void process_broker(FILE *log_file,
                           struct Broker *broker,
                           struct BufferController *bc,
                           int send_eventfd,
                           struct FdQueue *send_queue)
{
    struct BrokerMsg msg;
    if(pop_data_orch(broker, &msg, sizeof(msg)) < 0)
    {
        log_error(log_file, "[process_broker] pop_data_orch failed", 0);
        return;
    }

    if(msg.kind != BROKER_MSG_MATCH_RESULT)
    {
        log_error(log_file, "[process_broker] unexpected BrokerMsgKind", 0);
        return;
    }

    int fd = (int)msg.match_result.fd;

    if(!bc_is_output_free(bc, fd))
    {
        log_error(log_file, "[process_broker] output buffer occupied", 0);
        return;
    }

    uint8_t raw[TCP_SEGMENT_SIZE];
    memset(raw, 0, TCP_SEGMENT_SIZE);

    struct OutgoingPacket *pkt = (struct OutgoingPacket *)raw;
    pkt->event_type = msg.match_result.event_type;

    if(msg.match_result.event_type == SV_EVENT_MATCH_FOUND)
    {
        memcpy(pkt->game_id, msg.match_result.game_id, GAME_ID_SIZE);
        memcpy(pkt->player_id, msg.match_result.client_id, PLAYER_ID_SIZE);
        pkt->server_ip   = msg.match_result.server_ip;
        pkt->server_port = msg.match_result.server_port;
    }

    if(bc_push_output(bc, fd, raw, TCP_SEGMENT_SIZE) < 0)
    {
        log_error(log_file, "[process_broker] bc_push_output failed", 0);
        return;
    }

    fdq_push(send_queue, fd);

    uint64_t sig = 1;
    if(write(send_eventfd, &sig, sizeof(sig)) < 0)
    {
        log_error(log_file, "[process_broker] send_eventfd write failed", errno);
        return;
    }

    char log_msg[256];
    snprintf(log_msg, sizeof(log_msg),
        "[process_broker] fd=%d event_type=%u queued for send",
        fd, msg.match_result.event_type);
    log_message(log_file, log_msg);
}

static void send_data(  FILE *log_file,
                        struct BufferController *bc,
                        struct ConnController *cc,
                        int send_eventfd,
                        struct FdQueue *send_queue
                     )
{
    int fd = fdq_pop(send_queue);
    if(fd < 0)
    {
        log_error(log_file, "[send_data] send_queue empty", 0);
        return;
    }

    uint8_t buf[TCP_SEGMENT_SIZE];
    int n = bc_copy_output(bc, fd, buf, TCP_SEGMENT_SIZE);
    if(n < 0)
    {
        log_error(log_file, "[send_data] bc_copy_output failed", 0);
        return;
    }
    if(n == 0) return;

    int sent = tcp_send(log_file, fd, buf, (size_t)n);
    if(sent == -1)
    {
        log_error(log_file, "[send_data] tcp_send fatal error", 0);
        return;
    }

    bc_update_output(bc, fd, (size_t)sent);

    if(!bc_is_output_free(bc, fd))
    {
        fdq_push(send_queue, fd);
        uint64_t sig = 1;
        if(write(send_eventfd, &sig, sizeof(sig)) < 0)
        {
            log_error(
                log_file, 
                "[send_data] send_eventfd write failed", 
                errno
            );
        }
    }

    uint32_t ip = cc_get_ipv4(cc, fd);
    char str_ip[INET_ADDRSTRLEN] = "IP_ERROR";
    if(ip != 0) inet_ntop(AF_INET, &ip, str_ip, INET_ADDRSTRLEN);

    char log_msg[256];
    snprintf(log_msg, 256,
        "[send_data] fd=%d | ip=%s | bytes_sent=%d",
        fd, str_ip, sent);
    log_message(log_file, log_msg);
}

static void process_usr_request(FILE *log_file,
                                int efd,
                                struct BufferController *bc,
                                struct ConnController *cc,
                                struct Broker *broker,
                                int matchmaker_eventfd,
                                struct FdQueue *recv_queue
                            )
{
    int fd = fdq_pop(recv_queue);
    if(fd < 0)
    {
        log_error(log_file,
            "[process_usr_request] recv_queue empty",
            0);
        return;
    }

    struct IncomingPacket pkt;
    int n = bc_copy_input(bc, fd, &pkt, sizeof(pkt));
    if(n < 0)
    {
        log_error(log_file, "[process_usr_request] bc_copy_input failed", 0);
        return;
    }

    uint8_t *cid = cc_get_client_id(cc, fd, log_file);
    if(!cid) return;

    struct BrokerMsg msg;
    msg.kind = BROKER_MSG_CLIENT_REQUEST;
    memcpy(msg.client_request.client_id, cid, PLAYER_ID_SIZE);
    msg.client_request.fd         = (int32_t)fd;
    msg.client_request.event_type = (uint8_t)pkt.event_type;
    free(cid);

    if(push_data_sessions_man(broker, &msg, sizeof(msg)) < 0)
    {
        log_error(log_file,
            "[process_usr_request] push_data_sessions_man failed",
            0);
        return;
    }

    uint64_t sig = 1;
    if(write(matchmaker_eventfd, &sig, sizeof(sig)) < 0)
        log_error(log_file,
            "[process_usr_request] matchmaker_eventfd write failed",
            errno);

    struct epoll_event ev = {
        .data.fd = fd,
        .events  = EPOLLIN | EPOLLRDHUP | EPOLLHUP | EPOLLERR,
    };
    if(epoll_ctl(efd, EPOLL_CTL_MOD, fd, &ev) < 0)
        log_error(log_file,
            "[process_usr_request] epoll_ctl MOD (resume) failed",
            errno);

    char log_msg[256];
    snprintf(
        log_msg, 256,
         "[process_usr_request] fd=%d request processed",
         fd
        );
    log_message(log_file, log_msg);
}

static void read_incoming_data( FILE *log_file,
                                int fd,
                                int efd,
                                int recv_eventfd,
                                struct FdQueue *recv_queue,
                                struct BufferController *bc
                            )
{
    size_t space = bc_input_available_space(bc, fd);

    if(space == 0)
    {
        /**
         * Buffer full — previous request not yet consumed
         * Pause reads for this fd
         */
        struct epoll_event ev = {
            .data.fd = fd,
            .events  = EPOLLRDHUP | EPOLLHUP | EPOLLERR,
        };

        if(epoll_ctl(efd, EPOLL_CTL_MOD, fd, &ev) < 0)
            log_error(
                log_file,
                "[read_incoming_data] epoll_ctl MOD (pause) failed",
                errno
            );

        return;
    }

    uint8_t tmp[space];
    int n = tcp_read(log_file, fd, tmp, space);

    /**
         * 0 = EAGAIN/EINTR
         * -1 = error (logged inside tcp_read)
         * -2 = let EPOLLRDHUP handle it
    */
    if(n <= 0)
        return;

    if(bc_push_input(bc, fd, tmp, (size_t)n) < 0)
    {
        log_error(log_file, "[read_incoming_data] bc_push_input failed", 0);
        return;
    }

    if(bc_input_available_space(bc, fd) == 0)
    {
        /**
         * Buffer now full — a complete request is ready
         * Push fd onto recv_queue, signal recv_eventfd once (semaphore += 1)
         */
        fdq_push(recv_queue, fd);
        uint64_t sig = 1;
        if(write(recv_eventfd, &sig, sizeof(sig)) < 0)
        {
            log_error(log_file,
                "[read_incoming_data] recv_eventfd write failed",
                errno
            );
        }

        struct epoll_event ev = {
            .data.fd = fd,
            .events  = EPOLLRDHUP | EPOLLHUP | EPOLLERR,
        };
        if(epoll_ctl(efd, EPOLL_CTL_MOD, fd, &ev) < 0)
        {
            log_error(log_file,
                "[read_incoming_data] epoll_ctl MOD (suspend) failed",
                errno
            );
        }
    }

    char log_msg[256];
    snprintf(
        log_msg, 256,
         "[read_incoming_data] fd=%d bytes received: %d",
         fd, n
    );
    log_message(log_file, log_msg);
}

static void disconnect_handler( FILE *log_file,
                                struct ConnController *cc,
                                struct BufferController *bc,
                                int fd
                            )
{
    uint32_t ip = cc_get_ipv4(cc, fd);
    char str_ip[INET_ADDRSTRLEN] = "IP_ERROR";

    if(ip != 0) inet_ntop(AF_INET, &ip, str_ip, INET_ADDRSTRLEN);

    cc_close_connection(cc, fd, log_file);
    bc_remove(bc, fd);

    char log_msg[256];
    snprintf(
        log_msg,
        256,
        "[disconnect_handler] Clinet Disconnected | fd: %d | ip: %s",
        fd,
        str_ip
    );
    log_message(log_file, log_msg);
}

static int process_events( int n_events,
                    int lfd,
                    int efd,
                    int send_eventfd,
                    int recv_eventfd,
                    struct OrchArgs *orch_args,
                    struct BufferController *c_buf,
                    struct ConnController *c_con,
                    struct FdQueue *recv_queue,
                    struct FdQueue *send_queue,
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
            uint64_t val;
            read(fd, &val, sizeof(val));
            process_broker(
                orch_args->log_file,
                orch_args->broker,
                c_buf,
                send_eventfd,
                send_queue
            );

            continue;
        }

        if(fd == send_eventfd)
        {
            uint64_t val;
            read(fd, &val, sizeof(val));
            send_data(orch_args->log_file, c_buf, c_con, send_eventfd, send_queue);

            continue;
        }

        if(fd == recv_eventfd)
        {
            uint64_t val;
            read(fd, &val, sizeof(val));
            process_usr_request(
                orch_args->log_file,
                efd,
                c_buf,
                c_con,
                orch_args->broker,
                orch_args->matchmaker_eventfd,
                recv_queue
            );

            continue;
        }

        // Client fd
        if(events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR))
        {
            disconnect_handler(
                orch_args->log_file,
                c_con,
                c_buf,
                fd
            );

            continue;
        }

        if(events & EPOLLIN)
        {
            read_incoming_data(
                orch_args->log_file,
                fd,
                efd,
                recv_eventfd,
                recv_queue,
                c_buf
            );
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
    struct FdQueue *recv_queue     = NULL;
    struct FdQueue *send_queue     = NULL;
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

    recv_queue = fdq_init();
    if(!recv_queue)
    {
        log_error(log, "[orch_run_t] fdq_init (recv) failed", errno);
        goto exit;
    }

    send_queue = fdq_init();
    if(!send_queue)
    {
        log_error(log, "[orch_run_t] fdq_init (send) failed", errno);
        goto exit;
    }

    send_eventfd = eventfd(0, EFD_NONBLOCK | EFD_SEMAPHORE);
    if(send_eventfd < 0)
    {
        log_error(log, "[orch_run_t] send_eventfd failed", errno);
        goto exit;
    }

    recv_eventfd = eventfd(0, EFD_NONBLOCK | EFD_SEMAPHORE);
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
            if(errno != EINTR)
            {
                log_error(
                    log, 
                    "[orch_run_t] epoll_wait critical failure", 
                    errno
                );
                goto exit;
            }
        }

        if(ret == 0) continue;

        int status = process_events(
            ret,
            lfd,
            efd,
            send_eventfd,
            recv_eventfd,
            t_orch_args,
            c_buf,
            c_con,
            recv_queue,
            send_queue,
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
    if(recv_queue) fdq_destroy(recv_queue);
    if(send_queue) fdq_destroy(send_queue);
    if(lfd != -1) close(lfd);
    if(efd != -1) close(efd);
    if(send_eventfd != -1) close(send_eventfd);
    if(recv_eventfd != -1) close(recv_eventfd);

    return NULL;
}
