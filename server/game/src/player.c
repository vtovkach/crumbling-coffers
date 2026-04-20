#include "player.h"

<<<<<<< HEAD
#include <stdlib.h>
#include <string.h>

struct Player *create_player(uint8_t *player_id, uint8_t player_idx, FILE *log_file)
{
    (void)log_file;

    struct Player *player = calloc(1, sizeof(struct Player));
    if (!player) return NULL;

    memcpy(player->player_id, player_id, PLAYER_ID_SIZE);
    player->player_idx = player_idx;

    return player;
}

void update_player(struct Player *player, const struct ClientRegularPacket *pkt)
{
    player->pos_x = pkt->pos_x;
    player->pos_y = pkt->pos_y;
    player->vel_x = pkt->vel_x;
    player->vel_y = pkt->vel_y;
    player->score = pkt->score;
=======
struct Player *create_player(uint8_t *player_id, FILE *log_file)
{
    return NULL;
>>>>>>> 4ff7ed2 (Squash in PROJ-151-create-packetization-utility-for-game-server (pull request #70))
}

void destroy_player(struct Player *player, FILE *log_file)
{
<<<<<<< HEAD
    (void)log_file;

    if (!player) return;

    free(player->items);
    free(player);
}
=======
}

void update_player(struct Player *player)
{
}
>>>>>>> 4ff7ed2 (Squash in PROJ-151-create-packetization-utility-for-game-server (pull request #70))
