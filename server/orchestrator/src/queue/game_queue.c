#include <stdio.h>                          
#include <unistd.h>      
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <arpa/inet.h>

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

struct ClientSessionInfo
{
    char ip[INET_ADDRSTRLEN];
    uint16_t port; 
    int fd; 
    uint8_t player_id[PLAYER_ID_SIZE];
};

static int compare_func(void *left, void *right)
{
    struct Client *left_c = *(struct Client **)left;
    struct Client *right_c = *(struct Client **)right; 

    return (left_c->client_id > right_c->client_id) - 
           (left_c->client_id < right_c->client_id);
}

static void logSessionCreated(FILE *log_file,
                              const uint8_t *game_id,
                              const struct ClientSessionInfo *clients,
                              size_t count)
{
    char msg[2048];
    size_t offset = 0;

    offset += snprintf(msg + offset, sizeof(msg) - offset,
                       "Game session created with game_id: ");

    for (size_t i = 0; i < GAME_ID_SIZE; i++)
        offset += snprintf(msg + offset, sizeof(msg) - offset,
                           "%02x", game_id[i]);

    offset += snprintf(msg + offset, sizeof(msg) - offset, "\n");

    for (size_t i = 0; i < count; i++) {
        offset += snprintf(msg + offset, sizeof(msg) - offset,
                           "    -- Player #%d : fd=%d ip=%s port=%u player_id=",
                           (int )i + 1,
                           clients[i].fd,
                           clients[i].ip,
                           clients[i].port);

        for (size_t j = 0; j < PLAYER_ID_SIZE; j++)
            offset += snprintf(msg + offset, sizeof(msg) - offset,
                               "%02x", clients[i].player_id[j]);

        offset += snprintf(msg + offset, sizeof(msg) - offset, "\n");
    }

    log_message(log_file, msg);
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
    gq->cur_size = 0;

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
    if(gq->cur_size == gq->max_capacity)
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

    gq->cur_size++;

    return 0;
}

int removeClientFromQueue(struct GameQueue *const gq, struct Client *const client)
{
    if (gq->cur_size == 0)
        return 0;

    avl__remove_internal(gq->gameQueue, (void *)&client);
    gq->cur_size--;

    return 0;
}

struct Client *retrieveClientFromQueue(struct GameQueue *const gq, FILE *const log_file)
{
    if(gq->cur_size == 0)
    {
        log_message(log_file, "[retrieveClientFromQueue] game queue --> 0");
        return NULL;
    }

    struct Client **avl_client_entry = (struct Client **)find_min(gq->gameQueue);
    if(!avl_client_entry)
    {
        char msg[128];
        snprintf(msg, 128, "[retrieveClientFromQueue] find_min --> NULL \
            | cur_size = %lu", gq->cur_size);
        log_message(log_file, msg);
        return NULL;
    }
    
    struct Client *client = *avl_client_entry;

    avl__remove_internal(gq->gameQueue, (void *)avl_client_entry);
    gq->cur_size--;

    // Retrieve the client with lowest admission id 
    return client;
}

bool gq_ready(struct GameQueue *const gq, unsigned int clients)
{
    // Returns true when the game queue contains the required number of clients
    return gq->cur_size == clients;
}

int formSession(FILE *const log_file, struct GameQueue *const gq, int epoll_fd,
                uint16_t available_port, uint32_t ip)
{
    uint8_t game_id[GAME_ID_SIZE] = {0};

    if (!secure_random_bytes(game_id, GAME_ID_SIZE))
        return -1;

    struct ClientSessionInfo clients_info[PLAYERS_PER_MATCH];

    for (int i = 0; i < PLAYERS_PER_MATCH; i++)
    {
        struct Client *cur_client = retrieveClientFromQueue(gq, log_file);
        if (!cur_client)
        {
            log_message(log_file, "Error: [formSession] retrieveClientFromQueue failed.");
            return -1;
        }

        uint8_t player_id[PLAYER_ID_SIZE] = {0};
        if (!secure_random_bytes(player_id, PLAYER_ID_SIZE))
        {
            log_message(log_file, "Error: [formSession] random generator failed.");
            return -1;
        }

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

        // Update monitored events for this client fd
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
            log_message(log_file, "Error: [formSession] epoll_ctl failed.");
            return -1;
        }

        // Save clients info for future logging 
        clients_info[i].fd = cur_client->fd;
        inet_ntop(AF_INET, &cur_client->addr.sin_addr, clients_info[i].ip, INET_ADDRSTRLEN); // Retrieve string ip  
        clients_info[i].port = ntohs(cur_client->addr.sin_port); 
        memcpy(clients_info[i].player_id, player_id, PLAYER_ID_SIZE);
    }

    logSessionCreated(log_file, game_id, clients_info, PLAYERS_PER_MATCH);

    return 0;
}