#ifndef _POST_OFFICE_
#define _POST_OFFICE_

#include <stdint.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>

struct Mailbox;
struct PostOffice;

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

int post_office_mail_drop_push(struct PostOffice *po, 
                               const void *packet);

int post_office_mail_drop_pop(struct PostOffice *po, 
                              void *out_packet, 
                              size_t out_size);

#endif