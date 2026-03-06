#include <stdio.h>
#include <unistd.h>        
#include <string.h>        
#include <stdint.h>
#include "ds/hashmap.h" 

unsigned int hash(const void *key, unsigned int table_size)
{
    uint32_t x;
    memcpy(&x, key, sizeof(uint32_t));

    // Murmur3 finalizer mix
    x ^= x >> 16;
    x *= 0x85ebca6b;
    x ^= x >> 13;
    x *= 0xc2b2ae35;
    x ^= x >> 16;

    return x & (table_size - 1);  // table_size must be power of 2
}

int ht_close_all_sockets(HashTable *hash)
{
    if(!hash)
        return -1;

    int closed_count = 0;

    for(unsigned int i = 0; i < hash->hash_table_size; ++i)
    {
        Node *cur = hash->hash_table[i];

        while(cur)
        {
            if(cur->key)
            {
                int fd;
                memcpy(&fd, cur->key, sizeof(int));

                if(fd >= 0)
                {
                    if(close(fd) == 0)
                        closed_count++;
                }
            }

            cur = cur->nextNode;
        }
    }

    return closed_count;
}
