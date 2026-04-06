#ifndef CONN_CONTROLLER_H
#define CONN_CONTROLLER_H

#include <stdio.h>
#include <stdint.h>

struct ConnController;

struct ConnController *cc_init(int epoll_fd);
void cc_destroy(struct ConnController *cc);

int *cc_accept_connection(struct ConnController *cc, int listen_fd, FILE *log_file, int *out_count);
int cc_close_connection(struct ConnController *cc, int fd, FILE *log_file);

uint8_t     *cc_get_client_id(const struct ConnController *cc, int fd, FILE *log_file);
uint32_t     cc_get_ipv4(const struct ConnController *cc, int fd);
unsigned int cc_connection_count(const struct ConnController *cc);

#endif
