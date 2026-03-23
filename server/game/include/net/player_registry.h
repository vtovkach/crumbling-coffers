#ifndef _PLAYER_REGISTRY_
#define _PLAYER_REGISTRY_ 

#include "ds/hashmap.h"

#include <stdint.h>
#include <stddef.h>
#include <sys/socket.h>
#include <netinet/in.h>

struct PlayerEntry;
struct PlayersRegistry;

struct PlayersRegistry *players_registry_create(size_t max_players);

void players_registry_destroy(struct PlayersRegistry *pr);

int players_registry_add(struct PlayersRegistry *pr,
                         const uint8_t *player_id,
                         uint8_t player_index,
                         struct sockaddr_in addr);

int players_registry_get_index(struct PlayersRegistry *pr,
                                const uint8_t *player_id,
                                uint8_t *out_index);

struct sockaddr_in *players_registry_get_addr(struct PlayersRegistry *pr,
                                            const uint8_t *player_id);

int players_registry_seq_set_by_id(struct PlayersRegistry *pr,
                                   const uint8_t *player_id,
                                   uint32_t new_seqnum);

int players_registry_seq_set_by_index(struct PlayersRegistry *pr,
                                      uint8_t player_index,
                                      uint32_t new_seqnum);

uint32_t *players_registry_seq_get_by_id(struct PlayersRegistry *pr,
                                        const uint8_t *player_id);

uint32_t *players_registry_seq_get_by_index(struct PlayersRegistry *pr,
                                           uint8_t player_index);

#endif 