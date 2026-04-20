#include "game.h"

struct Game *create_game(uint8_t *game_id, uint16_t map_id, size_t players_num, FILE *log_file)
{
    return NULL;
}

void destroy_game(struct Game *game, FILE *log_file)
{
}

void add_player(struct Game *game, struct Player *player)
{
}

void update_game(struct Game *game)
{
}

struct Packet *form_auth_packet(struct Game *game)
{
    return NULL;
}
