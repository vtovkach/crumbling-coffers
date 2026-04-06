#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>
#include <stdint.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "orchestrator/conn_controller.h"
#include "ds/hashmap.h"
#include "random.h"
#include "log_system.h"
#include "server-config.h"

struct Client
{
    uint8_t            client_id[PLAYER_ID_SIZE];
    int                fd;
    struct sockaddr_in addr;
    struct timespec    ts;
};

struct ConnController
{
    HashTable    *clients;
    int           epoll_fd;
    unsigned int  num_connections;
};

static unsigned int hash(const void *key, unsigned int table_size)
{
    uint32_t x;
    memcpy(&x, key, sizeof(uint32_t));

    x ^= x >> 16;
    x *= 0x85ebca6b;
    x ^= x >> 13;
    x *= 0xc2b2ae35;
    x ^= x >> 16;

    return x & (table_size - 1);
}

struct ConnController *cc_init(int epoll_fd)
{
    struct ConnController *cc = malloc(sizeof(*cc));
    if (!cc) return NULL;

    cc->clients = ht_create(sizeof(int), 1,
                            sizeof(struct Client), 1,
                            hash, HASH_TABLE_SIZE);
    if (!cc->clients)
    {
        free(cc);
        return NULL;
    }

    cc->epoll_fd        = epoll_fd;
    cc->num_connections = 0;

    return cc;
}

void cc_destroy(struct ConnController *cc)
{
    if (!cc) return;

    for (unsigned int i = 0; i < cc->clients->hash_table_size; i++)
    {
        Node *cur = cc->clients->hash_table[i];
        while (cur)
        {
            int fd;
            memcpy(&fd, cur->key, sizeof(int));
            if (fd >= 0)
            {
                epoll_ctl(cc->epoll_fd, EPOLL_CTL_DEL, fd, NULL);
                close(fd);
            }
            cur = cur->nextNode;
        }
    }

    ht_destroy(cc->clients);
    free(cc);
}

int *cc_accept_connection(struct ConnController *cc, int listen_fd, FILE *log_file, int *out_count)
{
    *out_count = 0;

    int *fds = malloc(MAX_TCP_QUEUE * sizeof(int));
    if (!fds) return NULL;

    int accepted = 0;

    while (true)
    {
        struct Client      new_client = {0};
        struct sockaddr_in c_addr;
        socklen_t          ca_len = sizeof(c_addr);

        int conn_fd = accept4(listen_fd, (struct sockaddr *)&c_addr,
                              &ca_len, SOCK_NONBLOCK | SOCK_CLOEXEC);
        if (conn_fd < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK) break;
            log_error(log_file, "[cc_accept_connection] accept4 error", errno);
            continue;
        }

        if (!secure_random_bytes(new_client.client_id, PLAYER_ID_SIZE))
        {
            log_error(log_file, "[cc_accept_connection] secure_random_bytes failed", 0);
            close(conn_fd);
            continue;
        }

        clock_gettime(CLOCK_MONOTONIC, &new_client.ts);
        new_client.fd   = conn_fd;
        new_client.addr = c_addr;

        if (ht__insert_internal(cc->clients, &conn_fd, &new_client) < 0)
        {
            log_error(log_file, "[cc_accept_connection] ht insert failed", 0);
            close(conn_fd);
            continue;
        }

        struct epoll_event ev = {
            .data.fd = conn_fd,
            .events  = EPOLLIN | EPOLLRDHUP | EPOLLHUP | EPOLLERR,
        };

        if (epoll_ctl(cc->epoll_fd, EPOLL_CTL_ADD, conn_fd, &ev) == -1)
        {
            int saved_errno = errno;
            ht__remove_internal(cc->clients, &conn_fd);
            close(conn_fd);
            log_error_fd(log_file, "[cc_accept_connection] epoll_ctl ADD failed",
                         conn_fd, saved_errno);
            continue;
        }

        cc->num_connections++;
        fds[accepted++] = conn_fd;

        char ip_str[INET_ADDRSTRLEN];
        char msg[128];
        inet_ntop(AF_INET, &c_addr.sin_addr, ip_str, sizeof(ip_str));
        snprintf(msg, sizeof(msg), "[cc_accept_connection] CONNECT %s:%u fd=%d",
                 ip_str, ntohs(c_addr.sin_port), conn_fd);
        log_message(log_file, msg);
    }

    if (accepted == 0)
    {
        free(fds);
        return NULL;
    }

    int *result = realloc(fds, accepted * sizeof(int));
    *out_count = accepted;
    return result ? result : fds;
}

int cc_close_connection(struct ConnController *cc, int fd, FILE *log_file)
{
    struct Client *client = ht__get_internal(cc->clients, &fd, sizeof(int));
    if (!client)
    {
        log_error_fd(log_file, "[cc_close_connection] client not found", fd, 0);
        epoll_ctl(cc->epoll_fd, EPOLL_CTL_DEL, fd, NULL);
        close(fd);
        return -1;
    }

    if (epoll_ctl(cc->epoll_fd, EPOLL_CTL_DEL, fd, NULL) == -1)
        log_error_fd(log_file, "[cc_close_connection] epoll_ctl DEL failed", fd, errno);

    if (ht__remove_internal(cc->clients, &fd) <= 0)
        log_error_fd(log_file, "[cc_close_connection] ht_remove failed (possible leak)", fd, 0);

    cc->num_connections--;
    close(fd);

    return 0;
}

uint8_t *cc_get_client_id(const struct ConnController *cc, int fd, FILE *log_file)
{
    struct Client *client = ht__get_internal(cc->clients, &fd, sizeof(int));
    if(!client)
    {
        log_error_fd(log_file, "[cc_get_client_id] client not found", fd, 0);
        return NULL;
    }

    uint8_t *id = malloc(PLAYER_ID_SIZE);
    if(!id)
    {
        log_error(log_file, "[cc_get_client_id] malloc failed", errno);
        return NULL;
    }

    memcpy(id, client->client_id, PLAYER_ID_SIZE);
    return id;
}

uint32_t cc_get_ipv4(const struct ConnController *cc, int fd)
{
    struct Client *client = ht__get_internal(cc->clients, &fd, sizeof(int));
    if (!client) return 0;
    return client->addr.sin_addr.s_addr;
}

unsigned int cc_connection_count(const struct ConnController *cc)
{
    return cc->num_connections;
}
