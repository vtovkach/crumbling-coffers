#include "herald.h"

#include <stdlib.h>

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