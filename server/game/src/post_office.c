#include "post_office.h"
#include "server-config.h"

#include <stdlib.h>
#include <string.h>

struct Mailbox 
{
    uint8_t *packet_buf;
    size_t packet_size;
    pthread_mutex_t lock;
    atomic_bool ready;
};

struct PostOffice
{
    struct Mailbox *mailboxes;
    size_t players;
};

static void clean_mailboxes(struct Mailbox *mailboxes, size_t num_boxes)
{
    for(size_t i = 0; i < num_boxes; i++)
    {
        free(mailboxes[i].packet_buf);
        pthread_mutex_destroy(&mailboxes[i].lock);
    }
}

struct PostOffice *post_office_init(size_t mailbox_count)
{
    struct PostOffice *post_office = calloc(1, sizeof(*post_office));
    if(!post_office) return NULL; 

    post_office->mailboxes = calloc(mailbox_count, sizeof(struct Mailbox));
    if(!post_office->mailboxes) 
    {
        free(post_office);
        return NULL;
    }

    post_office->players = mailbox_count;

    // Initialize Mailboxes 
    for(size_t i = 0; i < mailbox_count; i++)
    {
        struct Mailbox *cur_mailbox = &post_office->mailboxes[i];
        cur_mailbox->packet_buf = calloc(1, UDP_DATAGRAM_SIZE);
        if(!cur_mailbox->packet_buf)
        {
            clean_mailboxes(post_office->mailboxes, i);
            free(post_office->mailboxes);
            free(post_office);
            return NULL;
        } 

        cur_mailbox->packet_size = UDP_DATAGRAM_SIZE;
        atomic_init(&cur_mailbox->ready, false);
        pthread_mutex_init(&cur_mailbox->lock, NULL);
    }

    return post_office;
}

void post_office_destroy(struct PostOffice *post_office)
{
    if(!post_office) return;
    clean_mailboxes(post_office->mailboxes, post_office->players);
    free(post_office->mailboxes);
    free(post_office);
}

bool post_office_is_ready(const struct PostOffice *post_office,
                          size_t player_index)
{
    return atomic_load_explicit(
        &post_office->mailboxes[player_index].ready, 
        memory_order_acquire
    );
}

int post_office_write(struct PostOffice *post_office,
                      size_t player_index,
                      const void *src,
                      size_t size)
{
    if(player_index >= post_office->players) return -1; // Incorrect index 

    struct Mailbox *target_mailbox = &post_office->mailboxes[player_index];

    if(size > target_mailbox->packet_size) return -1; // Incorrect size 

    pthread_mutex_lock(&target_mailbox->lock);
    memcpy(target_mailbox->packet_buf, src, size);
    atomic_store_explicit(&target_mailbox->ready, true, memory_order_release);
    pthread_mutex_unlock(&target_mailbox->lock);

    return 0;
}

int post_office_read(struct PostOffice *post_office,
                     size_t player_index,
                     void *dest,
                     size_t dest_capacity)
{
    // Check validity of the player index 
    if (player_index >= post_office->players) return -1;

    struct Mailbox *target_mailbox = &post_office->mailboxes[player_index];

    // Check if destination capacity matches the packet size of the mailbox 
    if (dest_capacity != target_mailbox->packet_size) return -1;

    if (!atomic_load_explicit(&target_mailbox->ready, memory_order_acquire))
        return 1; // Not ready 

    pthread_mutex_lock(&target_mailbox->lock);

    if (!atomic_load_explicit(&target_mailbox->ready, memory_order_relaxed)) 
    {
        pthread_mutex_unlock(&target_mailbox->lock);
        return 1;
    }

    memcpy(dest, target_mailbox->packet_buf, dest_capacity);
    atomic_store_explicit(&target_mailbox->ready, false, memory_order_release);

    pthread_mutex_unlock(&target_mailbox->lock);
    return 0;
}