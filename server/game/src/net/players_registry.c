#include "net/player_registry.h"
#include "ds/hashmap.h"
#include "server-config.h"

#include <stdlib.h>
#include <string.h>

struct PlayerEntry
{
    struct sockaddr_in addr;   // network address (IP + port)
    uint8_t index;          // internal player index
};

struct PlayersRegistry
{
    HashTable *id_to_entry; // maps player_id -> PlayerEntry
    uint32_t *seqnums;      // indexed by player index
    size_t players_count;
};

static unsigned int hash_function(const void *data, unsigned int table_size)
{
    const uint8_t *bytes = (const uint8_t *)data;

    uint32_t hash = 2166136261u; 

    for (size_t i = 0; i < PLAYER_ID_SIZE; i++) 
    {
        hash ^= bytes[i];
        hash *= 16777619u;
    }

    return hash % table_size;
}

struct PlayersRegistry *players_registry_create(size_t max_players)
{
    struct PlayersRegistry *pr = calloc(1, sizeof(*pr));
    if (!pr)
        return NULL;

    pr->seqnums = calloc(max_players, sizeof(*pr->seqnums));
    if (!pr->seqnums) 
    {
        free(pr);
        return NULL;
    }

    pr->players_count = max_players;

    pr->id_to_entry = ht_create(PLAYER_ID_SIZE, 
        1, 
        sizeof(struct PlayerEntry), 
        1, 
        hash_function, 
        pr->players_count * 2
    );

    if (!pr->id_to_entry) 
    {
        free(pr->seqnums);
        free(pr);
        return NULL;
    }
    
    return pr;
}

void players_registry_destroy(struct PlayersRegistry *pr)
{
    if (!pr) return;

    if (pr->id_to_entry) ht_destroy(pr->id_to_entry);

    free(pr->seqnums);
    free(pr);
}

int players_registry_add(struct PlayersRegistry *pr,
                         const uint8_t *player_id,
                         uint8_t player_index,
                         struct sockaddr_in addr)
{
    struct PlayerEntry entry;

    if (!player_id) 
        return -1;

    if (player_index >= pr->players_count) 
        return -1;

    memset(&entry, 0, sizeof(entry));
    entry.addr = addr;
    entry.index = player_index;

    return ht__insert_internal(pr->id_to_entry, player_id, &entry);
}

int players_registry_get_index(struct PlayersRegistry *pr,
                               const uint8_t *player_id,
                               uint8_t *out_index)
{
    struct PlayerEntry *entry;

    if (!player_id) 
        return -1;

    entry = ht__get_internal(pr->id_to_entry, player_id, PLAYER_ID_SIZE);
        
    if (!entry)
        return -1;

    *out_index = entry->index;
    return 0;
}

struct sockaddr_in *players_registry_get_addr(struct PlayersRegistry *pr,
                                           const uint8_t *player_id)
{
    struct PlayerEntry *entry;

    if (!player_id) 
        return NULL;

    entry = ht__get_internal(pr->id_to_entry, player_id, PLAYER_ID_SIZE);
    if (!entry)
        return NULL;

    return &entry->addr;
}

int players_registry_seq_set_by_id(struct PlayersRegistry *pr,
                                   const uint8_t *player_id,
                                   uint32_t new_seqnum)
{
    struct PlayerEntry *entry;

    if (!player_id)
        return -1;

    entry = ht__get_internal(pr->id_to_entry, player_id, PLAYER_ID_SIZE);
    if (!entry)
        return -1;

    if (entry->index >= pr->players_count)
        return -1;

    pr->seqnums[entry->index] = new_seqnum;
    return 0;
}

int players_registry_seq_set_by_index(struct PlayersRegistry *pr,
                                      uint8_t player_index,
                                      uint32_t new_seqnum)
{
    if (player_index >= pr->players_count)
        return -1;

    pr->seqnums[player_index] = new_seqnum;
    return 0;
}

uint32_t *players_registry_seq_get_by_id(struct PlayersRegistry *pr,
                                        const uint8_t *player_id)
{
    struct PlayerEntry *entry;

    if (!player_id)
        return NULL;

    entry = ht__get_internal(pr->id_to_entry, player_id, PLAYER_ID_SIZE);
    if (!entry)
        return NULL;

    return &pr->seqnums[entry->index];
}

uint32_t *players_registry_seq_get_by_index(struct PlayersRegistry *pr,
                                           uint8_t player_index)
{
    if (player_index >= pr->players_count)
        return NULL;

    return &pr->seqnums[player_index];
}
