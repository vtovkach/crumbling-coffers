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

}