#ifndef _POST_OFFICE_
#define _POST_OFFICE_

#include <stdint.h>
#include <pthread.h>
#include <stdatomic.h>

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

struct PostOffice *post_office_init(size_t mailbox_count);

void post_office_destroy(struct PostOffice *post_office);

bool post_office_is_ready(const struct PostOffice *post_office,
                          size_t player_index);

int post_office_write(struct PostOffice *post_office,
                      size_t player_index,
                      const void *src,
                      size_t size);

int post_office_read(struct PostOffice *post_office,
                     size_t player_index,
                     void *dest,
                     size_t dest_capacity);

#endif