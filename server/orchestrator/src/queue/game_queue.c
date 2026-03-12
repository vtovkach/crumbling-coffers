#include <stdio.h>                          
#include <unistd.h>      
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>

#include "orchestrator/state/client.h"
#include "server-config.h"            
#include "ds/ds_tree.h"
#include "orchestrator/queue/game_queue.h"
#include "log_system.h"
#include "random.h"

struct __attribute__((packed)) TCP_Game_Packet
{
    uint32_t ip;
    uint16_t port;
    uint8_t  game_id[GAME_ID_SIZE];
    uint8_t  player_id[PLAYER_ID_SIZE];
};

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

int addClientToQueue(struct GameQueue *const gq, struct Client *const client,
                     FILE *const log_file)
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
        log_error(log_file, "[addClientToQueue] failed inserting element \
                             into game queue.", 0);
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

    avl__remove_internal(gq->gameQueue, (void *)client);

    // Retrieve the client with lowest admission id 
    return *client;
}

bool gq_ready(struct GameQueue *const gq, unsigned int clients)
{
    unsigned int queue_size = avl__get_size(gq->gameQueue);

    // Returns true when the game queue contains the required number of clients
    return queue_size == clients;
}

int formSession(FILE *const log_file, struct GameQueue *const gq, int epoll_fd,
                uint16_t available_port, uint32_t ip)
{
    uint8_t game_id[GAME_ID_SIZE] = {0};

    if (!secure_random_bytes(game_id, GAME_ID_SIZE))
        return -1;

    for (int i = 0; i < PLAYERS_PER_MATCH; i++)
    {
        struct Client *cur_client = retrieveClientFromQueue(gq);
        if (!cur_client)
            return -1;

        uint8_t player_id[PLAYER_ID_SIZE] = {0};
        if (!secure_random_bytes(player_id, PLAYER_ID_SIZE))
            return -1;

        struct TCP_Game_Packet packet = {
            .ip   = ip,
            .port = available_port,
        };

        memcpy(packet.game_id, game_id, GAME_ID_SIZE);
        memcpy(packet.player_id, player_id, PLAYER_ID_SIZE);

        memcpy(cur_client->game_queue_info, &packet, sizeof(packet));

        ssize_t remaining_bytes = TCP_SEGMENT_SIZE - sizeof(packet);
        if (remaining_bytes > 0)
            memset(cur_client->game_queue_info + sizeof(packet), 0, remaining_bytes);

        cur_client->game_q_ready = true;

        // Add file descriptor to epoll monitoring 
        struct epoll_event ev = {
        .data.fd = cur_client->fd,
        .events =
              EPOLLOUT
            | EPOLLHUP
            | EPOLLERR
            | EPOLLRDHUP,
        };

        if(epoll_ctl(epoll_fd, EPOLL_CTL_MOD, cur_client->fd, &ev) < 0)
        {
            // Critical Error 
            return -1;
        }
    }

    return 0;
}