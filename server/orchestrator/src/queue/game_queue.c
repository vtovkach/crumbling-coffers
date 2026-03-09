#include <stdio.h>                          
#include <unistd.h>      
#include <sys/types.h>
#include <stdlib.h>

#include "orchestrator/state/client.h"
#include "server-config.h"            
#include "ds/ds_tree.h"
#include "orchestrator/queue/game_queue.h"
#include "log_system.h"

static int compare_func(void *left, void *right)
{
    struct Client *left_c = *(struct Client **)left;
    struct Client *right_c = *(struct Client **)right; 

    return (left_c->client_id > right_c->client_id) - 
           (left_c->client_id < right_c->client_id);
}

struct GameQueue *createGameQueue()
{   
    struct GameQueue *gq = malloc(sizeof(*gq));
    if(!gq)
        return NULL;

    gq->gameQueue = avl_create(sizeof(struct Client *), 1, compare_func);
    if(!gq->gameQueue)
    {
        free(gq);
        return NULL;
    }

    gq->max_capacity = MAX_GAME_QUEUE;

    return gq;
}

void freeGameQueue(struct GameQueue *gq)
{
    if(!gq) return; 

    avl_destroy(gq->gameQueue);
    free(gq);
}

int addClientToQueue(struct GameQueue *const gq, struct Client *const client)
{
    if(gq->gameQueue->num_elements == gq->max_capacity)
    {
        // Log error queue is full 
        return -1;
    }

    if(avl__insert_internal(gq->gameQueue, client) < 0)
    {
        // Log Error 
        return -1; 
    }
    
    return 0;
}

int removeClientFromQueue(struct GameQueue *const gq, struct Client *const client)
{
    if (gq->gameQueue->num_elements == 0)
        return 0;

    avl__remove_internal(gq->gameQueue, &client);

    return 0;
}

struct Client *retrieveClientFromQueue(struct GameQueue *const gq)
{
    if(gq->gameQueue->num_elements == 0)
        return NULL;

    struct Client **client = (struct Client **)find_min(gq->gameQueue);
    if(!client)
        return NULL;

    // Retrieve the client with lowest admission id 
    return *client;
}