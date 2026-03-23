#include "post_office.h"
#include "server-config.h"

#include <stdlib.h>


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
    clean_mailboxes(post_office->mailboxes, post_office->players);
    free(post_office->mailboxes);
    free(post_office);
}

bool post_office_is_ready(const struct PostOffice *post_office,
                          size_t player_index)
{

}

int post_office_write(const struct PostOffice *post_office,
                      size_t player_index,
                      const void *src,
                      size_t size)
{

}

int post_office_read(struct PostOffice *post_office,
                     size_t player_index,
                     void *dest,
                     size_t dest_capacity)
{

}