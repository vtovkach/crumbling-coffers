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

int addClientToQueue(struct GameQueue *const gq, struct Client *const client, FILE *const log_file)
{
    if((size_t)avl__get_size(gq->gameQueue) == gq->max_capacity)
    {
        // Log error queue is full 
        log_error(log_file, "[addClientToQueue] game queue is full.", 0);
        return -1;
    }
    
    if(avl__insert_internal(gq->gameQueue, (void *)&client) < 0)
    {
        // Log error 
        log_error(log_file, "[addClientToQueue] failed inserting element into game queue.", 0);
        return -1; 
    }

    return 0;
}

int removeClientFromQueue(struct GameQueue *const gq, struct Client *const client)
{
    if ((size_t)avl__get_size(gq->gameQueue) == 0)
        return 0;

    avl__remove_internal(gq->gameQueue, (void *)&client);

    return 0;
}

struct Client *retrieveClientFromQueue(struct GameQueue *const gq)
{
    if((size_t)avl__get_size(gq->gameQueue) == 0)
        return NULL;

    struct Client **client = (struct Client **)find_min(gq->gameQueue);
    if(!client)
        return NULL;

    // Retrieve the client with lowest admission id 
    return *client;
}

bool gq_ready(struct GameQueue *const gq, unsigned int clients)
{
    unsigned int queue_size = avl__get_size(gq->gameQueue);

    // Returns true when the game queue contains the required number of clients
    return queue_size == clients;
}

int formSession(FILE *const log_file, struct GameQueue *const gq, int available_port)
{
    
    


    log_message(log_file, "Session is created!\n");

    return 0;
}