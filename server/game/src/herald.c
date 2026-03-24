#include "herald.h"

#include <stdlib.h>
#include <string.h>

struct Herald
{
    uint8_t packet_buf[UDP_DATAGRAM_SIZE]; 
    size_t packet_size;
    pthread_mutex_t lock;
    atomic_bool ready;
};

struct Herald *herald_init()
{
    struct Herald *herald = calloc(1, sizeof(struct Herald));
    if(!herald) return NULL;

    herald->packet_size = UDP_DATAGRAM_SIZE;
    pthread_mutex_init(&herald->lock, NULL);
    atomic_init(&herald->ready, false); 

    return herald;
}

void herald_destroy(struct Herald *herald)
{
    if(!herald) return;
    pthread_mutex_destroy(&herald->lock);
    free(herald);
}

bool herald_is_ready(const struct Herald *herald)
{
    return atomic_load_explicit(
        &herald->ready, 
        memory_order_acquire
    );
}

int herald_write(struct Herald *herald, void *packet, size_t size)
{ 
    if(!packet || size != herald->packet_size) return -1;

    pthread_mutex_lock(&herald->lock);

    memcpy(herald->packet_buf, packet, size);
    atomic_store_explicit(&herald->ready, true, memory_order_release);

    pthread_mutex_unlock(&herald->lock);

    return 0;
}

int herald_read(struct Herald *herald, void *dest, size_t dest_size)
{
    if(!dest || dest_size != herald->packet_size) return -1;

    if(!atomic_load_explicit(&herald->ready, memory_order_acquire))
        return 1;

    pthread_mutex_lock(&herald->lock);

    memcpy(dest, herald->packet_buf, dest_size);
    atomic_store_explicit(&herald->ready, false, memory_order_release);

    pthread_mutex_unlock(&herald->lock);

    return 0;
}