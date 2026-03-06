#ifndef CLIENTS_TABLE_H
#define CLIENTS_TABLE_H

#include "ds/hashmap.h" 

unsigned int hash(const void *key, unsigned int table_size);

int ht_close_all_sockets(HashTable *clients);

#endif