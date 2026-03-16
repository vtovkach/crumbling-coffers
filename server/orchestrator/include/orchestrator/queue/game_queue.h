#ifndef _GAME_QUEUE_H
#define _GAME_QUEUE_H

#include <stdio.h>

#include "orchestrator/state/client.h"      
#include "ds/ds_tree.h"

#define GAME_ID_SIZE 16
#define PLAYER_ID_SIZE 16

struct GameQueue
{
    AVL_Tree *gameQueue;

    size_t cur_size; 
    size_t max_capacity;
};

struct GameQueue *createGameQueue();

void freeGameQueue(struct GameQueue *gq);

int addClientToQueue(struct GameQueue *const gq, struct Client *const client, FILE *const log_file);

struct Client *retrieveClientFromQueue(struct GameQueue *const gq, FILE *const log_file);

int removeClientFromQueue(struct GameQueue *const gq, struct Client *const client);

bool gq_ready(struct GameQueue *const gq, unsigned int clients);

int formSession(FILE *const log_file, struct GameQueue *const gq, int epoll_fd, uint16_t available_port, uint32_t ip);

#endif