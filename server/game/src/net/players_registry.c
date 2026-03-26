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
    size_t players_count;   // max players
    uint8_t next_index;     // next auto-assigned internal index
    uint8_t *allowed_player_ids;   // flat array: players_count * PLAYER_ID_SIZE
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

struct PlayersRegistry *players_registry_create(size_t max_players,
                                                const uint8_t *player_ids)
{
    struct PlayersRegistry *pr = calloc(1, sizeof(*pr));
    if (!pr)
        return NULL;

    if (!player_ids || max_players == 0)
    {
        free(pr);
        return NULL;
    }

    pr->seqnums = calloc(max_players, sizeof(*pr->seqnums));
    if (!pr->seqnums)
    {
        free(pr);
        return NULL;
    }

    pr->allowed_player_ids = malloc(max_players * PLAYER_ID_SIZE);
    if (!pr->allowed_player_ids)
    {
        free(pr->seqnums);
        free(pr);
        return NULL;
    }

    memcpy(pr->allowed_player_ids, player_ids, max_players * PLAYER_ID_SIZE);

    pr->players_count = max_players;
    pr->next_index = 0;

    pr->id_to_entry = ht_create(
        PLAYER_ID_SIZE,
        1,
        sizeof(struct PlayerEntry),
        1,
        hash_function,
        pr->players_count * 2
    );

    if (!pr->id_to_entry)
    {
        free(pr->allowed_player_ids);
        free(pr->seqnums);
        free(pr);
        return NULL;
    }

    return pr;
}

void players_registry_destroy(struct PlayersRegistry *pr)
{
    if (!pr) return;

    if (pr->id_to_entry)
        ht_destroy(pr->id_to_entry);

    free(pr->allowed_player_ids);
    free(pr->seqnums);
    free(pr);
}

int players_registry_add(struct PlayersRegistry *pr,
                         const uint8_t *player_id,
                         uint8_t player_index,
                         struct sockaddr_in addr)
{
    struct PlayerEntry entry;
    int ret;

    if (!pr || !player_id)
        return -1;

    if (player_index >= pr->players_count)
        return -1;

    memset(&entry, 0, sizeof(entry));
    entry.addr = addr;
    entry.index = player_index;

    ret = ht__insert_internal(pr->id_to_entry, player_id, &entry);
    if (ret < 0)
        return -1;

    return 0;
}

int players_registry_get_index(struct PlayersRegistry *pr,
                               const uint8_t *player_id,
                               uint8_t *out_index)
{
    struct PlayerEntry *entry = NULL;

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

static int player_id_is_allowed(const struct PlayersRegistry *pr,
                                const uint8_t *player_id)
{
    size_t i;

    if (!pr || !player_id || !pr->allowed_player_ids)
        return 0;

    for (i = 0; i < pr->players_count; i++)
    {
        const uint8_t *cur = pr->allowed_player_ids + (i * PLAYER_ID_SIZE);

        if (memcmp(cur, player_id, PLAYER_ID_SIZE) == 0)
            return 1;
    }

    return 0;
}

int players_registry_add_next(struct PlayersRegistry *pr,
                              const uint8_t *player_id,
                              struct sockaddr_in addr)
{
    int ret;

    if (!pr || !player_id)
        return -1;

    if (pr->next_index >= pr->players_count)
        return -1;

    if (!player_id_is_allowed(pr, player_id))
        return -1;

    ret = players_registry_add(
        pr,
        player_id,
        pr->next_index,
        addr
    );

    if (ret != 0)
        return -1;

    pr->next_index++;
    return 0;
}