#include "matchmaker/players_queue.h"
#include "matchmaker/player.h"
#include "ds/ds_tree.h"
#include "ds/hashmap.h"
#include "log_system.h"

#include <stdlib.h>
#include <string.h>

struct PlayersQueue
{
    AVL_Tree *players;
    size_t capacity;
    size_t cur_size;

    uint64_t seq_num;

    HashTable *fd_seq_mapping;

    FILE *log_file;
};

static int compare_players(void *left, void *right)
{
    uint64_t a = ((struct Player *)left)->seq_num;
    uint64_t b = ((struct Player *)right)->seq_num;
    if (a < b) return -1;
    if (a > b) return  1;
    return 0;
}

static unsigned int hash_fd(const void *key, unsigned int table_size)
{
    uint32_t x;
    memcpy(&x, key, sizeof(uint32_t));

    x ^= x >> 16;
    x *= 0x85ebca6b;
    x ^= x >> 13;
    x *= 0xc2b2ae35;
    x ^= x >> 16;

    return x & (table_size - 1);
}

/* Returns the smallest power of 2 >= n, minimum 16. */
static unsigned int next_pow2(size_t n)
{
    unsigned int p = 16;
    while (p < n) p <<= 1;
    return p;
}

struct PlayersQueue *pq_create(size_t size, FILE *log_file)
{
    struct PlayersQueue *pq = calloc(1, sizeof(*pq));
    if (!pq)
    {
        log_error(log_file, "pq_create: failed to allocate PlayersQueue", 0);
        return NULL;
    }

    pq->log_file = log_file;

    pq->players = avl_create(sizeof(struct Player), 1, compare_players);
    if (!pq->players)
    {
        log_error(log_file, "pq_create: failed to create AVL tree", 0);
        free(pq);
        return NULL;
    }

    pq->fd_seq_mapping = ht_create(sizeof(int), 1, sizeof(uint64_t), 1,
                                   hash_fd, next_pow2(size));
    if (!pq->fd_seq_mapping)
    {
        log_error(log_file, "pq_create: failed to create fd_seq_mapping", 0);
        avl_destroy(pq->players);
        free(pq);
        return NULL;
    }

    pq->capacity = size;
    pq->cur_size = 0;
    pq->seq_num  = 1;

    return pq;
}

void pq_destroy(struct PlayersQueue *pq)
{
    if (!pq) return;
    avl_destroy(pq->players);
    ht_destroy(pq->fd_seq_mapping);
    free(pq);
}

int pq_add_player(struct PlayersQueue *pq, const struct Player *player,
                  bool assign_seq_num)
{
    if (!pq || !player) return -1;

    if (pq->cur_size >= pq->capacity)
    {
        log_error(pq->log_file, "pq_add_player: queue is at capacity", 0);
        return -1;
    }

    struct Player p = *player;
    if (assign_seq_num)
        p.seq_num = pq->seq_num;

    if (avl__insert_internal(pq->players, &p) != 0)
    {
        log_error(pq->log_file, "pq_add_player: failed to insert into AVL tree", 0);
        return -1;
    }

    if (ht__insert_internal(pq->fd_seq_mapping, &p.fd, &p.seq_num) < 0)
    {
        log_error(pq->log_file, "pq_add_player: failed to insert into fd_seq_mapping", 0);
        avl__remove_internal(pq->players, &p);
        return -1;
    }

    if (assign_seq_num)
        pq->seq_num++;
    pq->cur_size++;
    return 0;
}

int pq_remove_player(struct PlayersQueue *pq, int fd)
{
    if (!pq) return -1;

    uint64_t *seq_ptr = ht__get_internal(pq->fd_seq_mapping, &fd, sizeof(int));
    if (!seq_ptr)
    {
        log_error(pq->log_file, "pq_remove_player: fd not found in fd_seq_mapping", 0);
        return -1;
    }

    struct Player key = { .seq_num = *seq_ptr };

    ht__remove_internal(pq->fd_seq_mapping, &fd);
    avl__remove_internal(pq->players, &key);

    pq->cur_size--;
    return 0;
}

int pop_from_queue(struct PlayersQueue *pq, struct Player *dest)
{
    if (!pq || !dest) return -1;

    if (pq->cur_size == 0)
    {
        log_error(pq->log_file, "pop_from_queue: queue is empty", 0);
        return -1;
    }

    struct Player *min = find_min(pq->players);
    if (!min)
    {
        log_error(pq->log_file, "pop_from_queue: find_min returned NULL on non-empty queue", 0);
        return -1;
    }

    *dest = *min;

    avl__remove_internal(pq->players, dest);
    ht__remove_internal(pq->fd_seq_mapping, &dest->fd);

    pq->cur_size--;
    return 0;
}

bool pq_ready(struct PlayersQueue *pq, size_t goal)
{
    if (!pq) return false;
    return pq->cur_size >= goal;
}
