#ifndef PLAYERS_QUEUE 
#define PLAYERS_QUEUE 

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "matchmaker/player.h"

struct PlayersQueue;

/**
 * Creates and initializes a PlayersQueue.
 *
 * @param size      Desired capacity of the queue.
 * @param log_file  Log file for error reporting.
 * @return          Pointer to initialized PlayersQueue on success, NULL on failure.
 */
struct PlayersQueue *pq_create(size_t size, FILE *log_file);

/**
 * Destroys a PlayersQueue and releases all associated resources.
 *
 * @param pq  Pointer to the PlayersQueue to destroy.
 */
void pq_destroy(struct PlayersQueue *pq);

/**
 * Adds a player to the queue.
 *
 * @param pq              Pointer to the PlayersQueue.
 * @param player          Pointer to the Player to add.
 * @param assign_seq_num  If true, overwrites player->seq_num with the queue's
 *                        internal counter and advances it. If false, uses
 *                        player->seq_num as-is and leaves the counter unchanged.
 * @return                0 on success, -1 on failure.
 */
int pq_add_player(struct PlayersQueue *pq, const struct Player *player,
                  bool assign_seq_num);
/**
 * Removes a player from the queue by file descriptor.
 *
 * @param pq  Pointer to the PlayersQueue.
 * @param fd  File descriptor identifying the player.
 * @return    0 on success, -1 if the player is not found or on error.
 */
int pq_remove_player(struct PlayersQueue *pq, int fd);

/**
 * Removes and returns the player with the smallest sequence number.
 *
 * @param pq    Pointer to the PlayersQueue.
 * @param dest  Output parameter where the removed player is stored.
 * @return      0 on success, -1 if the queue is empty or on error.
 */
int pop_from_queue(struct PlayersQueue *pq, struct Player *dest);

/**
 * Checks whether the queue contains at least `goal` players.
 *
 * @param pq    Pointer to the PlayersQueue.
 * @param goal  Minimum number of players required.
 * @return      true if the condition is satisfied, false otherwise.
 */
bool pq_ready(struct PlayersQueue *pq, size_t goal);

#endif