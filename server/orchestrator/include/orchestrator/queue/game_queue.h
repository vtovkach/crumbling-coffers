#ifndef _GAME_QUEUE_H
#define _GAME_QUEUE_H

#include <stdio.h>

#include "orchestrator/state/client.h"      
#include "ds/ds_tree.h"

struct GameQueue
{
    AVL_Tree *gameQueue;

    size_t max_capacity;
};

struct GameQueue *createGameQueue();

void freeGameQueue(struct GameQueue *gq);

int addClientToQueue(struct GameQueue *const gq, struct Client *const client, FILE *const log_file);

struct Client *retrieveClientFromQueue(struct GameQueue *const gq);

int removeClientFromQueue(struct GameQueue *const gq, struct Client *const client);

bool gq_ready(struct GameQueue *const gq, unsigned int clients);

int formSession(FILE *const log_file, struct GameQueue *const gq);

#endif